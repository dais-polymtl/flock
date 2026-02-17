#pragma once

#include <any>

#include "flock/core/common.hpp"
#include "flock/functions/input_parser.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/prompt_manager/prompt_manager.hpp"
#include <nlohmann/json.hpp>

namespace flock {

class ScalarFunctionBase {
public:
    ScalarFunctionBase() = delete;

    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<std::any> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);

    static nlohmann::json Complete(nlohmann::json& tuples, const std::string& user_prompt,
                                   ScalarFunctionType function_type, Model& model);
    static nlohmann::json BatchAndComplete(const nlohmann::json& tuples,
                                           const std::string& user_prompt_name, ScalarFunctionType function_type,
                                           Model& model);
};

}// namespace flock
