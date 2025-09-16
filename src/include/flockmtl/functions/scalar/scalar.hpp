#pragma once

#include <any>
#include <nlohmann/json.hpp>

#include "flockmtl/core/common.hpp"
#include "flockmtl/functions/input_parser.hpp"
#include "flockmtl/model_manager/model.hpp"
#include "flockmtl/prompt_manager/prompt_manager.hpp"

namespace flockmtl {

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

}// namespace flockmtl
