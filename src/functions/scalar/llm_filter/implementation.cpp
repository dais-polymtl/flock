#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "flock/core/config.hpp"
#include "flock/functions/scalar/llm_filter.hpp"
#include "flock/functions/scalar/scalar.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/model.hpp"

namespace flock {

duckdb::unique_ptr<duckdb::FunctionData> LlmFilter::Bind(
        duckdb::ClientContext& context,
        duckdb::ScalarFunction& bound_function,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments) {
    return ScalarFunctionBase::ValidateAndInitializeBindData(context, arguments, "llm_filter", false);
}


void LlmFilter::ValidateArguments(duckdb::DataChunk& args) {
    if (args.ColumnCount() < 2 || args.ColumnCount() > 3) {
        throw std::runtime_error("Invalid number of arguments.");
    }

    if (args.data[0].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Model details must be a string.");
    }
    if (args.data[1].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Prompt details must be a struct.");
    }

    if (args.ColumnCount() == 3 && args.data[2].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("Inputs must be a struct.");
    }
}

std::vector<std::string> LlmFilter::Operation(duckdb::DataChunk& args, LlmFunctionBindData* bind_data) {
    Model model = bind_data->CreateModel();

    auto model_details = model.GetModelDetails();
    MetricsManager::SetModelInfo(model_details.model_name, model_details.provider_name);

    auto prompt_context_json = CastVectorOfStructsToJson(args.data[1], args.size());
    auto context_columns = nlohmann::json::array();
    if (prompt_context_json.contains("context_columns")) {
        context_columns = prompt_context_json["context_columns"];
    }

    auto prompt = bind_data->prompt;

    std::vector<std::string> results;
    if (context_columns.empty()) {
        auto template_str = prompt;
        model.AddCompletionRequest(template_str, 1, OutputType::BOOL);
        auto response = model.CollectCompletions()[0]["items"][0];
        if (response.is_null()) {
            results.push_back("true");
        } else {
            results.push_back(response.dump());
        }
    } else {
        auto responses = BatchAndComplete(context_columns, prompt, ScalarFunctionType::FILTER, model);

        results.reserve(responses.size());
        for (const auto& response: responses) {
            if (response.is_null()) {
                results.emplace_back("true");
                continue;
            }
            results.push_back(response.dump());
        }
    }

    return results;
}

void LlmFilter::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    auto* db = context.db.get();
    const void* invocation_id = MetricsManager::GenerateUniqueId();

    MetricsManager::StartInvocation(db, invocation_id, FunctionType::LLM_FILTER);

    auto exec_start = std::chrono::high_resolution_clock::now();

    auto& func_expr = state.expr.Cast<duckdb::BoundFunctionExpression>();
    auto* bind_data = &func_expr.bind_info->Cast<LlmFunctionBindData>();

    const auto results = LlmFilter::Operation(args, bind_data);

    auto index = 0;
    for (const auto& res: results) {
        result.SetValue(index++, duckdb::Value(res));
    }

    auto exec_end = std::chrono::high_resolution_clock::now();
    double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
    MetricsManager::AddExecutionTime(exec_duration_ms);
}

}// namespace flock
