#include "flock/functions/scalar/llm_filter.hpp"

namespace flock {

void LlmFilter::ValidateArguments(duckdb::DataChunk& args) {
    if (args.ColumnCount() != 3) {
        throw std::runtime_error("Invalid number of arguments.");
    }

    if (args.data[0].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Model details must be a string.");
    }
    if (args.data[1].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Prompt details must be a struct.");
    }

    if (args.data[2].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Inputs must be a struct.");
    }
}

std::vector<std::string> LlmFilter::Operation(duckdb::DataChunk& args) {
    // LlmFilter::ValidateArguments(args);

    auto model_details_json = CastVectorOfStructsToJson(args.data[0], 1);
    Model model(model_details_json);
    auto prompt_context_json = CastVectorOfStructsToJson(args.data[1], args.size());
    auto context_columns = nlohmann::json::array();
    if (prompt_context_json.contains("context_columns")) {
        context_columns = prompt_context_json["context_columns"];
        prompt_context_json.erase("context_columns");
    }
    auto prompt_details = PromptManager::CreatePromptDetails(prompt_context_json);

    auto responses = BatchAndComplete(context_columns, prompt_details.prompt, ScalarFunctionType::FILTER, model);

    std::vector<std::string> results;
    results.reserve(responses.size());
    for (const auto& response: responses) {
        if (response.is_null()) {
            results.emplace_back("true");
            continue;
        }
        results.push_back(response.dump());
    }

    return results;
}

void LlmFilter::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    const auto results = LlmFilter::Operation(args);

    auto index = 0;
    for (const auto& res: results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

}// namespace flock
