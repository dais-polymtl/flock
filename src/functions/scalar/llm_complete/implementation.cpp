#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "flock/functions/scalar/llm_complete.hpp"
#include "flock/functions/scalar/scalar.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/model.hpp"


namespace flock {

duckdb::unique_ptr<duckdb::FunctionData> LlmComplete::Bind(
        duckdb::ClientContext& context,
        duckdb::ScalarFunction& bound_function,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments) {
    return ScalarFunctionBase::ValidateAndInitializeBindData(context, arguments, "llm_complete", false);
}


void LlmComplete::ValidateArguments(duckdb::DataChunk& args) {
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

std::vector<std::string> LlmComplete::Operation(duckdb::DataChunk& args, LlmFunctionBindData* bind_data) {
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
        model.AddCompletionRequest(template_str, 1, OutputType::STRING);
        auto completions = model.CollectCompletions();
        auto response = completions[0]["items"][0];
        if (response.is_string()) {
            results.push_back(response.get<std::string>());
        } else {
            results.push_back(response.dump());
        }
    } else {
        if (context_columns.empty()) {
            return results;
        }

        auto responses = BatchAndComplete(context_columns, prompt, ScalarFunctionType::COMPLETE, model);

        results.reserve(responses.size());
        for (const auto& response: responses) {
            if (response.is_string()) {
                results.push_back(response.get<std::string>());
            } else {
                results.push_back(response.dump());
            }
        }
    }
    return results;
}

void LlmComplete::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    auto* db = context.db.get();
    const void* invocation_id = MetricsManager::GenerateUniqueId();

    MetricsManager::StartInvocation(db, invocation_id, FunctionType::LLM_COMPLETE);

    auto exec_start = std::chrono::high_resolution_clock::now();

    auto& func_expr = state.expr.Cast<duckdb::BoundFunctionExpression>();
    auto* bind_data = &func_expr.bind_info->Cast<LlmFunctionBindData>();

    if (const auto results = LlmComplete::Operation(args, bind_data); static_cast<int>(results.size()) == 1) {
        auto empty_vec = duckdb::Vector(std::string());
        duckdb::UnaryExecutor::Execute<duckdb::string_t, duckdb::string_t>(
                empty_vec, result, args.size(),
                [&](duckdb::string_t name) { return duckdb::StringVector::AddString(result, results[0]); });
    } else {
        // Multiple results - one per row
        for (idx_t i = 0; i < results.size(); i++) {
            result.SetValue(i, duckdb::Value(results[i]));
        }
    }

    auto exec_end = std::chrono::high_resolution_clock::now();
    double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
    MetricsManager::AddExecutionTime(exec_duration_ms);
}

}// namespace flock
