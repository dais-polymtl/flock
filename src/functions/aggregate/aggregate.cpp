#include "flock/functions/aggregate/aggregate.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/prompt_manager/prompt_manager.hpp"
#include <duckdb/planner/expression/bound_function_expression.hpp>

namespace flock {

void AggregateFunctionBase::ValidateArgumentCount(
        const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
        const std::string& function_name) {
    if (arguments.size() != 2) {
        throw duckdb::BinderException(
                function_name + " requires 2 arguments: (1) model, (2) prompt with context_columns. Got " +
                std::to_string(arguments.size()));
    }
}

void AggregateFunctionBase::ValidateArgumentTypes(
        const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
        const std::string& function_name) {
    if (arguments[0]->return_type.id() != duckdb::LogicalTypeId::STRUCT) {
        throw duckdb::BinderException(function_name + ": First argument must be model (struct type)");
    }
    if (arguments[1]->return_type.id() != duckdb::LogicalTypeId::STRUCT) {
        throw duckdb::BinderException(
                function_name + ": Second argument must be prompt with context_columns (struct type)");
    }
}

AggregateFunctionBase::PromptStructInfo AggregateFunctionBase::ExtractPromptStructInfo(
        const duckdb::LogicalType& prompt_type) {
    PromptStructInfo info{false, std::nullopt, ""};

    for (idx_t i = 0; i < duckdb::StructType::GetChildCount(prompt_type); i++) {
        auto field_name = duckdb::StructType::GetChildName(prompt_type, i);
        if (field_name == "context_columns") {
            info.has_context_columns = true;
        } else if (field_name == "prompt" || field_name == "prompt_name") {
            if (!info.prompt_field_index.has_value()) {
                info.prompt_field_index = i;
                info.prompt_field_name = field_name;
            }
        }
    }

    return info;
}

void AggregateFunctionBase::ValidatePromptStructFields(const PromptStructInfo& info,
                                                       const std::string& function_name) {
    if (!info.has_context_columns) {
        throw duckdb::BinderException(
                function_name + ": Second argument must contain 'context_columns' field");
    }
}

void AggregateFunctionBase::InitializeModelJson(
        duckdb::ClientContext& context,
        const duckdb::unique_ptr<duckdb::Expression>& model_expr,
        LlmFunctionBindData& bind_data) {
    if (!model_expr->IsFoldable()) {
        return;
    }

    auto model_value = duckdb::ExpressionExecutor::EvaluateScalar(context, *model_expr);
    auto user_model_json = CastValueToJson(model_value);
    bind_data.model_json = Model::ResolveModelDetailsToJson(user_model_json);
}

void AggregateFunctionBase::InitializePrompt(
        duckdb::ClientContext& context,
        const duckdb::unique_ptr<duckdb::Expression>& prompt_expr,
        LlmFunctionBindData& bind_data) {
    nlohmann::json prompt_json;

    if (prompt_expr->IsFoldable()) {
        auto prompt_value = duckdb::ExpressionExecutor::EvaluateScalar(context, *prompt_expr);
        prompt_json = CastValueToJson(prompt_value);
    } else if (prompt_expr->expression_class == duckdb::ExpressionClass::BOUND_FUNCTION) {
        auto& func_expr = prompt_expr->Cast<duckdb::BoundFunctionExpression>();
        const auto& struct_type = prompt_expr->return_type;

        for (idx_t i = 0; i < duckdb::StructType::GetChildCount(struct_type) && i < func_expr.children.size(); i++) {
            auto field_name = duckdb::StructType::GetChildName(struct_type, i);
            auto& child = func_expr.children[i];

            if (field_name != "context_columns" && child->IsFoldable()) {
                try {
                    auto field_value = duckdb::ExpressionExecutor::EvaluateScalar(context, *child);
                    if (field_value.type().id() == duckdb::LogicalTypeId::VARCHAR) {
                        prompt_json[field_name] = field_value.GetValue<std::string>();
                    } else {
                        prompt_json[field_name] = CastValueToJson(field_value);
                    }
                } catch (...) {
                    // Skip fields that can't be evaluated
                }
            }
        }
    }

    auto prompt_details = PromptManager::CreatePromptDetails(prompt_json);
    bind_data.prompt = prompt_details.prompt;
}

duckdb::unique_ptr<LlmFunctionBindData> AggregateFunctionBase::ValidateAndInitializeBindData(
        duckdb::ClientContext& context,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
        const std::string& function_name) {

    ValidateArgumentCount(arguments, function_name);
    ValidateArgumentTypes(arguments, function_name);

    const auto& prompt_type = arguments[1]->return_type;
    auto prompt_info = ExtractPromptStructInfo(prompt_type);
    ValidatePromptStructFields(prompt_info, function_name);

    auto bind_data = duckdb::make_uniq<LlmFunctionBindData>();

    InitializeModelJson(context, arguments[0], *bind_data);
    InitializePrompt(context, arguments[1], *bind_data);

    return bind_data;
}

std::tuple<nlohmann::json, nlohmann::json>
AggregateFunctionBase::CastInputsToJson(duckdb::Vector inputs[], idx_t count) {
    auto prompt_context_json = CastVectorOfStructsToJson(inputs[1], count);
    auto context_columns = nlohmann::json::array();
    if (prompt_context_json.contains("context_columns")) {
        context_columns = prompt_context_json["context_columns"];
        prompt_context_json.erase("context_columns");
    } else {
        throw std::runtime_error("Missing 'context_columns' in second argument. The prompt struct must include context_columns.");
    }

    return std::make_tuple(prompt_context_json, context_columns);
}

}// namespace flock
