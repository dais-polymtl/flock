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
std::optional<nlohmann::json> RowToJudge(const BatchContext& batch, const size_t row_offset) {
    auto row = batch.Row(row_offset);
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
    if (request.function_type != ScalarFunctionType::FILTER) {
        throw std::runtime_error("llm_complete is not supported by the 'typesafe' provider: Jev answers typed "
                                 "questions and cannot generate text. Use llm_filter, or point this function at a "
                                 "generative provider.");
    }

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

    PendingBatch batch{request.batch.RowCount(), request.threshold.value_or(model_details_.threshold), {}};

    auto rows = nlohmann::json::object();
    auto questions = nlohmann::json::object();

    for (size_t row_offset = 0; row_offset < batch.row_count; row_offset++) {
        auto row = RowToJudge(request.batch, row_offset);
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

std::vector<nlohmann::json> TypeSafeProvider::CollectCompletions(const std::string& contentType) {
    const auto raw_responses = model_handler_->CollectCompletions(contentType);

    std::vector<nlohmann::json> results;
    results.reserve(pending_batches_.size());

    size_t raw_index = 0;
    for (const auto& batch: pending_batches_) {
        if (batch.asked_rows.empty()) {
            results.push_back(nlohmann::json{{"items", UnansweredItems(batch.row_count)}});
            continue;
        }

        if (raw_index >= raw_responses.size()) {
            throw std::runtime_error("TypeSafe returned fewer responses than requests were queued");
        }
        const auto& response = raw_responses[raw_index++];

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

    pending_batches_.clear();
    return results;
}

}// namespace flock
