#include "flock/functions/aggregate/aggregate.hpp"

namespace flock {

void AggregateFunctionBase::ValidateArguments(duckdb::Vector inputs[], idx_t input_count) {
    if (input_count != 3) {
        throw std::runtime_error("Expected exactly 3 arguments for aggregate function, got " + std::to_string(input_count));
    }

    if (inputs[0].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Expected a struct type for model details");
    }

    if (inputs[1].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Expected a struct type for prompt details");
    }

    if (inputs[2].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Expected a struct type for prompt inputs");
    }
}

std::tuple<nlohmann::json, nlohmann::json, nlohmann::json>
AggregateFunctionBase::CastInputsToJson(duckdb::Vector inputs[], idx_t count) {
    auto model_details_json = CastVectorOfStructsToJson(inputs[0], 1);
    auto prompt_context_json = CastVectorOfStructsToJson(inputs[1], count);
    auto context_columns = nlohmann::json::array();
    if (prompt_context_json.contains("context_columns")) {
        context_columns = prompt_context_json["context_columns"];
        prompt_context_json.erase("context_columns");
    } else {
        throw std::runtime_error("Expected 'context_columns' in prompt details");
    }

    return std::make_tuple(model_details_json, prompt_context_json, context_columns);
}

}// namespace flock
