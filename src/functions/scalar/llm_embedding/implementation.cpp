#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "flock/core/config.hpp"
#include "flock/functions/scalar/llm_embedding.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/model.hpp"

namespace flock {

duckdb::unique_ptr<duckdb::FunctionData> LlmEmbedding::Bind(
        duckdb::ClientContext& context,
        duckdb::ScalarFunction& bound_function,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments) {
    return ScalarFunctionBase::ValidateAndInitializeBindData(context, arguments, "llm_embedding", true, false);
}


void LlmEmbedding::ValidateArguments(duckdb::DataChunk& args) {
    if (args.ColumnCount() < 2 || args.ColumnCount() > 2) {
        throw std::runtime_error("LlmEmbedScalarParser: Invalid number of arguments.");
    }
    if (args.data[0].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("LlmEmbedScalarParser: Model details must be a struct.");
    }
    if (args.data[1].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("LlmEmbedScalarParser: Inputs must be a struct.");
    }
}

std::vector<duckdb::vector<duckdb::Value>> LlmEmbedding::Operation(duckdb::DataChunk& args, LlmFunctionBindData* bind_data) {
    auto inputs = CastVectorOfStructsToJson(args.data[1], args.size());
    for (const auto& item: inputs.items()) {
        if (item.key() != "context_columns") {
            throw std::runtime_error(duckdb_fmt::format("Unexpected key in inputs: {}", item.key()));
        }
    }
    for (const auto& context_column: inputs["context_columns"]) {
        if (context_column.contains("type") && context_column["type"].get<std::string>() == "image") {
            throw std::runtime_error("Image embedding is not supported yet. Please use text data for embedding.");
        }
    }

    Model model = bind_data->CreateModel();

    auto model_details = model.GetModelDetails();
    MetricsManager::SetModelInfo(model_details.model_name, model_details.provider_name);

    std::vector<std::string> prepared_inputs;
    auto num_rows = inputs["context_columns"][0]["data"].size();
    for (size_t row_idx = 0; row_idx < num_rows; row_idx++) {
        std::string concat_input;
        for (auto& context_column: inputs["context_columns"]) {
            concat_input += context_column["data"][row_idx].get<std::string>() + " ";
        }
        prepared_inputs.push_back(concat_input);
    }

    auto batch_size = model.GetModelDetails().batch_size;

    if (batch_size == 0 || batch_size > prepared_inputs.size()) {
        batch_size = static_cast<int>(prepared_inputs.size());
    }

    for (size_t i = 0; i < prepared_inputs.size(); i += batch_size) {
        std::vector<std::string> batch_inputs;
        for (size_t j = i; j < i + batch_size && j < prepared_inputs.size(); j++) {
            batch_inputs.push_back(prepared_inputs[j]);
        }
        model.AddEmbeddingRequest(batch_inputs);
    }

    std::vector<duckdb::vector<duckdb::Value>> results;
    auto all_embeddings = model.CollectEmbeddings();
    for (size_t index = 0; index < all_embeddings.size(); index++) {
        for (auto& embedding: all_embeddings[index]) {
            duckdb::vector<duckdb::Value> formatted_embedding;
            for (auto& value: embedding) {
                formatted_embedding.push_back(duckdb::Value(static_cast<double>(value)));
            }
            results.push_back(formatted_embedding);
        }
    }
    return results;
}

void LlmEmbedding::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    auto* db = context.db.get();
    const void* invocation_id = MetricsManager::GenerateUniqueId();

    MetricsManager::StartInvocation(db, invocation_id, FunctionType::LLM_EMBEDDING);

    auto exec_start = std::chrono::high_resolution_clock::now();

    auto& func_expr = state.expr.Cast<duckdb::BoundFunctionExpression>();
    auto* bind_data = &func_expr.bind_info->Cast<LlmFunctionBindData>();

    auto results = LlmEmbedding::Operation(args, bind_data);

    auto index = 0;
    for (const auto& res: results) {
        result.SetValue(index++, duckdb::Value::LIST(res));
    }

    auto exec_end = std::chrono::high_resolution_clock::now();
    double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
    MetricsManager::AddExecutionTime(exec_duration_ms);
}

}// namespace flock
