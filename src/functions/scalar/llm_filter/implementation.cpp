#include "flockmtl/functions/scalar/llm_filter.hpp"

namespace flockmtl {

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

std::vector<std::string> LlmFilter::Operation(duckdb::DataChunk& args, ExecutionMode mode) {
    // LlmFilter::ValidateArguments(args);

    auto model_details_json = CastVectorOfStructsToJson(args.data[0], 1)[0];
    Model model(model_details_json);
    auto prompt_details_json = CastVectorOfStructsToJson(args.data[1], 1)[0];
    auto prompt_details = PromptManager::CreatePromptDetails(prompt_details_json);

    auto tuples = CastVectorOfStructsToJson(args.data[2], args.size());

    nlohmann::json responses;
    switch (mode) {
        case ExecutionMode::SYNC:
            responses = BatchAndComplete<ExecutionMode::SYNC>(tuples, prompt_details.prompt, ScalarFunctionType::FILTER, model);
            break;
        case ExecutionMode::ASYNC:
            responses = BatchAndComplete<ExecutionMode::ASYNC>(tuples, prompt_details.prompt, ScalarFunctionType::FILTER, model);
            break;
        default:
            break;
    }

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

template<ExecutionMode MODE>
void LlmFilter::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    const auto results = LlmFilter::Operation(args, MODE);

    auto index = 0;
    for (const auto& res: results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

template void LlmFilter::Execute<ExecutionMode::SYNC>(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
template void LlmFilter::Execute<ExecutionMode::ASYNC>(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);

}// namespace flockmtl
