#include "flock/functions/scalar/llm_complete.hpp"
#include "flock/metrics/metrics.hpp"

#include <chrono>

namespace flock {

void LlmComplete::ValidateArguments(duckdb::DataChunk& args) {
    if (args.ColumnCount() < 2 || args.ColumnCount() > 3) {
        throw std::runtime_error("Invalid number of arguments.");
    }

    if (args.data[0].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Model details must be a string.");
    }
    if (args.data[1].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Prompt details must be a struct.");
    }

    if (args.ColumnCount() == 3) {
        if (args.data[2].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
            throw std::runtime_error("Inputs must be a struct.");
        }
    }
}

std::vector<std::string> LlmComplete::Operation(duckdb::DataChunk& args) {
    // LlmComplete::ValidateArguments(args);
    auto model_details_json = CastVectorOfStructsToJson(args.data[0], 1);
    Model model(model_details_json);
    auto prompt_context_json = CastVectorOfStructsToJson(args.data[1], args.size());
    auto context_columns = nlohmann::json::array();
    if (prompt_context_json.contains("context_columns")) {
        context_columns = prompt_context_json["context_columns"];
        prompt_context_json.erase("context_columns");
    }
    auto prompt_details = PromptManager::CreatePromptDetails(prompt_context_json);

    std::vector<std::string> results;
    if (context_columns.empty()) {
        auto template_str = prompt_details.prompt;
        model.AddCompletionRequest(template_str, 1, OutputType::STRING);
        auto response = model.CollectCompletions()[0]["items"][0];
        if (response.is_string()) {
            results.push_back(response.get<std::string>());
        } else {
            results.push_back(response.dump());
        }
    } else {
        if (context_columns.empty()) {
            return results;
        }

        auto responses = BatchAndComplete(context_columns, prompt_details.prompt, ScalarFunctionType::COMPLETE, model);

        results.reserve(responses.size());
        for (const auto& response: responses) {
            if (response.is_string()) {
                results.push_back(response.get<std::string>());
            } else {
                results.push_back(response.dump());
            }
        }
    }
    return results;
}

void LlmComplete::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    // Start execution timing
    auto exec_start = std::chrono::high_resolution_clock::now();

    if (const auto results = LlmComplete::Operation(args); static_cast<int>(results.size()) == 1) {
        auto empty_vec = duckdb::Vector(std::string());
        duckdb::UnaryExecutor::Execute<duckdb::string_t, duckdb::string_t>(
                empty_vec, result, args.size(),
                [&](duckdb::string_t name) { return duckdb::StringVector::AddString(result, results[0]); });
    } else {
        auto index = 0;
        for (const auto& res: results) {
            result.SetValue(index++, duckdb::Value(res));
        }
    }

    // End execution timing and update metrics
    auto exec_end = std::chrono::high_resolution_clock::now();
    double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
    FlockMetrics::GetInstance().AddExecutionTime(exec_duration_ms);
}

}// namespace flock
