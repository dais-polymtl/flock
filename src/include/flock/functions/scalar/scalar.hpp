#pragma once

#include <any>
#include <optional>

#include "flock/core/common.hpp"
#include "flock/functions/input_parser.hpp"
#include "flock/functions/llm_function_bind_data.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/prompt_manager/prompt_manager.hpp"
#include <nlohmann/json.hpp>

namespace flock {

class ScalarFunctionBase {
private:
    struct PromptStructInfo {
        bool has_context_columns;
        std::optional<idx_t> prompt_field_index;
        std::string prompt_field_name;
    };

    static void ValidateArgumentCount(const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
                                      const std::string& function_name);

    static void ValidateArgumentTypes(const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
                                      const std::string& function_name);

    static PromptStructInfo ExtractPromptStructInfo(const duckdb::LogicalType& prompt_type);

    static void ValidatePromptStructFields(const PromptStructInfo& info, const std::string& function_name, bool require_context_columns);

    static void InitializeModelJson(duckdb::ClientContext& context,
                                    const duckdb::unique_ptr<duckdb::Expression>& model_expr,
                                    LlmFunctionBindData& bind_data);

    static void InitializePrompt(duckdb::ClientContext& context,
                                 const duckdb::unique_ptr<duckdb::Expression>& prompt_expr,
                                 LlmFunctionBindData& bind_data);

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

    static duckdb::unique_ptr<LlmFunctionBindData> ValidateAndInitializeBindData(
            duckdb::ClientContext& context,
            duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
            const std::string& function_name,
            bool require_context_columns = true,
            bool initialize_prompt = true);
};

}// namespace flock
