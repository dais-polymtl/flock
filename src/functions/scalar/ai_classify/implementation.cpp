#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "flock/core/config.hpp"
#include "flock/functions/input_parser.hpp"
#include "flock/functions/scalar/ai_classify.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/model.hpp"

namespace flock {

duckdb::unique_ptr<duckdb::FunctionData> AiClassify::Bind(
        duckdb::ClientContext& context,
        duckdb::ScalarFunction& bound_function,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments) {
    auto bind_data = ScalarFunctionBase::ValidateAndInitializeBindData(context, arguments, "ai_classify", true);
    // 'return_probabilities' makes the result a struct instead of the bare label.
    if (arguments[1]->expression_class == duckdb::ExpressionClass::BOUND_FUNCTION) {
        const auto& children = arguments[1]->Cast<duckdb::BoundFunctionExpression>().children;
        for (idx_t i = 0; i < children.size(); i++) {
            if (duckdb::StructType::GetChildName(arguments[1]->return_type, i) == "return_probabilities" &&
                duckdb::ExpressionExecutor::EvaluateScalar(context, *children[i]).GetValue<bool>()) {
                bound_function.return_type = duckdb::LogicalType::STRUCT(
                        {{"choice", duckdb::LogicalType::VARCHAR},
                         {"confidence", duckdb::LogicalType::DOUBLE},
                         {"probabilities", duckdb::LogicalType::MAP(duckdb::LogicalType::VARCHAR, duckdb::LogicalType::DOUBLE)}});
            }
        }
    }
    return bind_data;
}

void AiClassify::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    const void* invocation_id = MetricsManager::GenerateUniqueId();
    MetricsManager::StartInvocation(context.db.get(), invocation_id, FunctionType::AI_CLASSIFY);
    auto exec_start = std::chrono::high_resolution_clock::now();

    auto& func_expr = state.expr.Cast<duckdb::BoundFunctionExpression>();
    auto* bind_data = &func_expr.bind_info->Cast<LlmFunctionBindData>();
    Model model = bind_data->CreateModel();
    auto model_details = model.GetModelDetails();
    MetricsManager::SetModelInfo(model_details.model_name, model_details.provider_name);

    // The choices are the same for every row, so the first row's are used.
    auto choices = nlohmann::json::object();
    const auto first_prompt = args.data[1].GetValue(0);
    const auto& prompt_type = first_prompt.type();
    for (idx_t i = 0; i < duckdb::StructType::GetChildCount(prompt_type); i++) {
        if (duckdb::StructType::GetChildName(prompt_type, i) != "choice") {
            continue;
        }
        // Each choice is a label, or a struct {label, description}.
        for (const auto& choice: duckdb::ListValue::GetChildren(duckdb::StructValue::GetChildren(first_prompt)[i])) {
            if (choice.type().id() != duckdb::LogicalTypeId::STRUCT) {
                choices[choice.ToString()] = nullptr;
                continue;
            }
            const auto fields = CastValueToJson(choice);
            choices[fields.at("label").get<std::string>()] = fields.value("description", nlohmann::json());
        }
    }

    const auto context_columns = CastVectorOfStructsToJson(args.data[1], args.size())["context_columns"];
    const auto responses = BatchAndComplete(context_columns, bind_data->prompt, ScalarFunctionType::CLASSIFY, model, choices);
    const auto with_probabilities = result.GetType().id() == duckdb::LogicalTypeId::STRUCT;
    for (idx_t i = 0; i < args.size(); i++) {
        const auto& answer = responses[i];
        if (!answer.is_object()) {
            result.SetValue(i, duckdb::Value());
            continue;
        }
        const auto choice = duckdb::Value(answer["choice"].get<std::string>());
        if (!with_probabilities) {
            result.SetValue(i, choice);
            continue;
        }
        duckdb::vector<duckdb::Value> labels;
        duckdb::vector<duckdb::Value> probabilities;
        const auto answer_probabilities = answer.value("probabilities", nlohmann::json::object());
        for (const auto& [label, probability]: answer_probabilities.items()) {
            labels.emplace_back(label);
            probabilities.push_back(duckdb::Value::DOUBLE(probability.get<double>()));
        }
        result.SetValue(i, duckdb::Value::STRUCT({{"choice", choice},
                                                  {"confidence", duckdb::Value::DOUBLE(answer.value("confidence", 0.0))},
                                                  {"probabilities", duckdb::Value::MAP(duckdb::LogicalType::VARCHAR, duckdb::LogicalType::DOUBLE,
                                                                                       std::move(labels), std::move(probabilities))}}));
    }

    auto exec_end = std::chrono::high_resolution_clock::now();
    MetricsManager::AddExecutionTime(std::chrono::duration<double, std::milli>(exec_end - exec_start).count());
}

}// namespace flock
