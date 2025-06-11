#include "flockmtl/functions/scalar/llm_complete_json.hpp"

namespace flockmtl {

void LlmCompleteJson::ValidateArguments(duckdb::DataChunk& args) {
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

std::vector<std::string> LlmCompleteJson::Operation(duckdb::DataChunk& args) {
    // LlmCompleteJson::ValidateArguments(args);

    auto model_details_json = CastVectorOfStructsToJson(args.data[0], 1)[0];
    Model model(model_details_json);
    auto prompt_details_json = CastVectorOfStructsToJson(args.data[1], 1)[0];
    auto prompt_details = PromptManager::CreatePromptDetails(prompt_details_json);

    std::vector<std::string> results;
    if (args.ColumnCount() == 2) {
        auto template_str = prompt_details.prompt;
        template_str += "\nThe Ouput should be in JSON format.";
        auto response = model.CallComplete(template_str);

        results.push_back(response.dump());
    } else {
        auto tuples = CastVectorOfStructsToJson(args.data[2], args.size());

        auto responses = BatchAndComplete(tuples, prompt_details.prompt, ScalarFunctionType::COMPLETE_JSON, model);

        results.reserve(responses.size());
        for (const auto& response: responses) {
            results.push_back(response.dump());
        }
    }
    return results;
}

void LlmCompleteJson::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {

    if (const auto results = LlmCompleteJson::Operation(args); static_cast<int>(results.size()) == 1) {
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
}

}// namespace flockmtl
