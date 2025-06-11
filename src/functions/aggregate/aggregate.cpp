#include "flockmtl/functions/aggregate/aggregate.hpp"

namespace flockmtl {

AggregateFunctionBase* AggregateFunctionBase::instance = nullptr;

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

std::tuple<nlohmann::json, nlohmann::json, std::vector<nlohmann::json>>
AggregateFunctionBase::CastInputsToJson(duckdb::Vector inputs[], idx_t count) {
    auto model_details_json = CastVectorOfStructsToJson(inputs[0], 1)[0];
    auto prompt_details_json = CastVectorOfStructsToJson(inputs[1], 1)[0];
    auto tuples = CastVectorOfStructsToJson(inputs[2], count);

    return std::make_tuple(model_details_json, prompt_details_json, tuples);
}

}// namespace flockmtl
