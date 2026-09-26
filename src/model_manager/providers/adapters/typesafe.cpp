#include "flock/model_manager/providers/adapters/typesafe.hpp"
#include <optional>
#include <stdexcept>
#include <string>

namespace flock {

namespace {

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

    const auto classify = request.function_type == ScalarFunctionType::CLASSIFY;
    if (!classify && !model_details_.threshold.has_value()) {
        throw std::runtime_error("llm_filter on the 'typesafe' provider needs a 'threshold' on the model.");
    }
    if (classify && request.choices.empty()) {
        throw std::runtime_error("ai_classify needs a 'choice' list in the prompt.");
    }

    PendingBatch batch{request.batch.RowCount(), model_details_.threshold.value_or(0.0), {}};

    auto rows = nlohmann::json::object();
    auto questions = nlohmann::json::object();

    for (size_t row_offset = 0; row_offset < batch.row_count; row_offset++) {
        auto row = RowToJudge(request.batch.Row(row_offset));
        if (!row) {
            // we skip null rows
            continue;
        }
        const auto key = std::to_string(row_offset);
        rows[key] = std::move(*row);
        // The key is not sent to the model, so the instructions name the row.
        if (classify) {
            questions[key] = {{"type", "choice"},
                              {"instructions", "Choose the label for row " + key + ", following the task stated in the state."},
                              {"criteria", request.choices}};
        } else {
            questions[key] = {{"type", "noul"},
                              {"instructions", "Row " + key + " satisfies the criterion stated in the state."}};
        }
        batch.asked_rows.push_back(row_offset);
    }

    pending_batches_.push_back(std::move(batch));

    // A batch with no questions is not sent (all rows are nulls)
    if (questions.empty()) {
        return;
    }

    model_handler_->AddRequest({{"model", model_details_.model},
                                {"state", {{classify ? "task" : "criterion", request.user_prompt}, {"rows", rows}}},
                                {"questions", questions}});
}

std::vector<nlohmann::json> TypeSafeProvider::CollectCompletions(const std::string& contentType) {
    const auto batches = std::move(pending_batches_);
    pending_batches_.clear();
    const auto raw_responses = model_handler_->CollectCompletions(contentType);

    std::vector<nlohmann::json> results;
    results.reserve(batches.size());

    size_t raw_index = 0;
    for (const auto& batch: batches) {
        if (batch.asked_rows.empty()) {
            results.push_back(nlohmann::json{{"items", nlohmann::json(batch.row_count, nullptr)}});
            continue;
        }
        const auto& response = raw_responses[raw_index++];

        if (IsTokenLimitExceededMarker(response)) {
            results.push_back(response);
            continue;
        }

        // Unless Jev answered for this exact row, it will have null value
        auto items = nlohmann::json(batch.row_count, nullptr);

        if (response.contains("answers") && response["answers"].is_object()) {
            const auto& answers = response["answers"];
            for (const auto row_offset: batch.asked_rows) {
                const auto key = std::to_string(row_offset);
                if (!answers.contains(key)) {
                    continue;
                }
                const auto& answer = answers[key];
                if (answer.contains("noul") && answer["noul"].is_number()) {
                    items[row_offset] = answer["noul"].get<double>() >= batch.threshold;
                } else if (answer.contains("choice") && answer["choice"].is_string()) {
                    items[row_offset] = answer;
                }
            }
        }

        results.push_back(nlohmann::json{{"items", items}});
    }

    return results;
}

}// namespace flock
