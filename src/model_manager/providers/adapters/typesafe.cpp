#include "flock/model_manager/providers/adapters/typesafe.hpp"
#include <optional>
#include <stdexcept>
#include <string>

namespace flock {

namespace {

// A question's key, naming the row by its offset in the batch.
std::string RowKey(const size_t row_offset) {
    return "r" + std::to_string(row_offset);
}

// The row with missing values as null, or nothing if every value is missing.
std::optional<nlohmann::json> RowToJudge(nlohmann::json row) {
    auto has_content = false;
    for (auto& item: row.items()) {
        // Flock hands every provider a missing context value as the string "NULL".
        const auto& value = item.value();
        if (value.is_null() || (value.is_string() && value.get_ref<const std::string&>() == "NULL")) {
            item.value() = nullptr;
        } else {
            has_content = true;
        }
    }
    if (!has_content) {
        return std::nullopt;
    }
    return row;
}

// Added by llm_first and llm_last to identify rows; never shown to Jev.
constexpr const char* ROW_ID_COLUMN = "flock_row_id";

// One null per row: the verdict of every row that has not been answered.
nlohmann::json UnansweredItems(const size_t num_rows) {
    return nlohmann::json(num_rows, nullptr);
}

}// namespace

void TypeSafeProvider::AddCompletionRequest(const std::string&, const int, OutputType, const nlohmann::json&) {
    throw std::runtime_error("TypeSafe builds its requests from structured tuples and takes no rendered prompt");
}

void TypeSafeProvider::AddEmbeddingRequest(const std::vector<std::string>&) {
    throw std::runtime_error("llm_embedding is not supported by the 'typesafe' provider: TypeSafe serves no "
                             "embeddings endpoint. Use an embedding model from another provider.");
}

void TypeSafeProvider::AddTranscriptionRequest(const nlohmann::json&) {
    throw std::runtime_error("Audio context is not supported by the 'typesafe' provider: TypeSafe accepts text "
                             "state only. Use a transcription model from another provider.");
}

void TypeSafeProvider::AddStructuredCompletionRequest(const StructuredCompletionRequest& request) {
    for (const auto& column: request.batch.Columns()) {
        if (!column.contains("type") || !column["type"].is_string()) {
            continue;
        }
        const auto type = column["type"].get<std::string>();
        if (type == "image" || type == "audio") {
            throw std::runtime_error("The 'typesafe' provider accepts text context only, but a context column has type '" +
                                     type + "'. Jev reads no images or audio.");
        }
    }

    if (const auto* scalar = std::get_if<ScalarFunctionType>(&request.function_type);
        scalar != nullptr && *scalar == ScalarFunctionType::FILTER) {
        AddVerdictRequest(request);
        return;
    }
    if (const auto* aggregate = std::get_if<AggregateFunctionType>(&request.function_type);
        aggregate != nullptr && (*aggregate == AggregateFunctionType::FIRST || *aggregate == AggregateFunctionType::LAST)) {
        AddPickRequest(request, *aggregate);
        return;
    }
    throw std::runtime_error("The 'typesafe' provider serves llm_filter, llm_first and llm_last only: Jev answers "
                             "typed questions and cannot generate text. Point this function at a generative "
                             "provider.");
}

void TypeSafeProvider::AddVerdictRequest(const StructuredCompletionRequest& request) {
    PendingBatch batch{request.batch.RowCount(), request.threshold.value_or(model_details_.threshold), {}, std::nullopt};

    auto rows = nlohmann::json::object();
    auto questions = nlohmann::json::object();

    for (size_t row_offset = 0; row_offset < batch.row_count; row_offset++) {
        auto row = RowToJudge(request.batch.Row(row_offset));
        if (!row) {
            continue;
        }
        const auto key = RowKey(row_offset);
        rows[key] = std::move(*row);
        // The key is not sent to the model, so the instructions name the row.
        questions[key] = {{"type", "noul"},
                          {"instructions", "Row " + key + " satisfies the criterion stated in the state."}};
        batch.asked_rows.push_back(row_offset);
    }

    pending_batches_.push_back(std::move(batch));

    // A batch with no questions is not sent.
    if (questions.empty()) {
        return;
    }
    model_handler_->AddRequest({{"model", model_details_.model},
                                {"state", {{"criterion", request.user_prompt}, {"rows", rows}}},
                                {"questions", questions}});
}

void TypeSafeProvider::AddPickRequest(const StructuredCompletionRequest& request,
                                      const AggregateFunctionType function_type) {
    std::optional<std::vector<nlohmann::json>> row_ids;
    for (const auto& column: request.batch.Columns()) {
        if (column.contains("name") && column["name"] == ROW_ID_COLUMN && column.contains("data")) {
            row_ids = column["data"].get<std::vector<nlohmann::json>>();
        }
    }
    if (!row_ids) {
        throw std::logic_error("A pick request carries no flock_row_id column");
    }
    PendingBatch batch{request.batch.RowCount(), 0.0, {}, std::move(row_ids)};

    auto rows = nlohmann::json::object();
    // Options are bare row keys; the rows themselves are in the state.
    auto options = nlohmann::json::object();

    for (size_t row_offset = 0; row_offset < batch.row_count; row_offset++) {
        auto values = request.batch.Row(row_offset);
        values.erase(ROW_ID_COLUMN);
        auto row = RowToJudge(std::move(values));
        // A row with nothing in it is no candidate for either end.
        if (!row) {
            continue;
        }
        const auto key = RowKey(row_offset);
        rows[key] = std::move(*row);
        options[key] = nullptr;
        batch.asked_rows.push_back(row_offset);
    }

    pending_batches_.push_back(std::move(batch));

    // With one candidate, or none, the answer is already known.
    if (options.size() <= 1) {
        return;
    }
    const auto* instructions = function_type == AggregateFunctionType::FIRST
                                       ? "Pick the row that best satisfies the criterion stated in the state."
                                       : "Pick the row that least satisfies the criterion stated in the state.";
    model_handler_->AddRequest(
            {{"model", model_details_.model},
             {"state", {{"criterion", request.user_prompt}, {"rows", rows}}},
             {"questions", {{"pick", {{"type", "choice"}, {"instructions", instructions}, {"criteria", options}}}}}});
}

std::vector<nlohmann::json> TypeSafeProvider::CollectCompletions(const std::string& contentType) {
    const auto batches = std::move(pending_batches_);
    pending_batches_.clear();
    const auto raw_responses = model_handler_->CollectCompletions(contentType);

    std::vector<nlohmann::json> results;
    results.reserve(batches.size());

    size_t raw_index = 0;
    const auto next_response = [&]() -> const nlohmann::json& {
        if (raw_index >= raw_responses.size()) {
            throw std::runtime_error("TypeSafe returned fewer responses than requests were queued");
        }
        return raw_responses[raw_index++];
    };

    // The answer to a pick: the flock_row_id of the chosen row.
    const auto picked_row_id = [&](const PendingBatch& batch) -> nlohmann::json {
        const auto& ids = *batch.row_ids;
        // Not sent: the only candidate wins, or the first row when none has content.
        if (batch.asked_rows.size() <= 1) {
            return ids[batch.asked_rows.empty() ? 0 : batch.asked_rows.front()];
        }
        const auto& response = next_response();
        // The aggregate shrinks and retries an oversized pick.
        if (IsTokenLimitExceededMarker(response)) {
            throw TokenLimitExceededError();
        }
        const auto choice = response.at("answers").at("pick").at("choice").get<std::string>();
        for (const auto row_offset: batch.asked_rows) {
            if (RowKey(row_offset) == choice) {
                return ids[row_offset];
            }
        }
        throw std::runtime_error("TypeSafe chose '" + choice + "', which is not one of the rows it was offered");
    };

    for (const auto& batch: batches) {
        if (batch.row_ids.has_value()) {
            results.push_back(nlohmann::json{{"items", {picked_row_id(batch)}}});
            continue;
        }

        if (batch.asked_rows.empty()) {
            results.push_back(nlohmann::json{{"items", UnansweredItems(batch.row_count)}});
            continue;
        }
        const auto& response = next_response();

        // A batch the caller must retry smaller passes straight through.
        if (IsTokenLimitExceededMarker(response)) {
            results.push_back(response);
            continue;
        }

        // Scatter answers back to their rows; skipped and unanswered rows stay null.
        auto items = UnansweredItems(batch.row_count);

        if (response.contains("answers") && response["answers"].is_object()) {
            const auto& answers = response["answers"];
            for (const auto row_offset: batch.asked_rows) {
                const auto key = RowKey(row_offset);
                if (!answers.contains(key)) {
                    continue;
                }
                const auto& answer = answers[key];
                if (answer.contains("noul") && answer["noul"].is_number()) {
                    items[row_offset] = answer["noul"].get<double>() >= batch.threshold;
                }
            }
        }

        results.push_back(nlohmann::json{{"items", items}});
    }

    return results;
}

}// namespace flock
