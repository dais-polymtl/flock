#include "flock/metrics/data_structures.hpp"
#include "flock/metrics/manager.hpp"
#include <vector>

namespace flock {

// Thread-local storage definitions (must be in .cpp file)
thread_local duckdb::DatabaseInstance* MetricsManager::current_db_ = nullptr;
thread_local const void* MetricsManager::current_state_id_ = nullptr;
thread_local FunctionType MetricsManager::current_function_type_ = FunctionType::UNKNOWN;

void MetricsManager::ExecuteGetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    auto* db = context.db.get();

    auto& metrics_manager = GetForDatabase(db);
    auto metrics = metrics_manager.GetMetrics();

    auto json_str = metrics.dump();

    result.SetVectorType(duckdb::VectorType::CONSTANT_VECTOR);
    auto result_data = duckdb::ConstantVector::GetData<duckdb::string_t>(result);
    result_data[0] = duckdb::StringVector::AddString(result, json_str);
}

void MetricsManager::ExecuteGetDebugMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    auto* db = context.db.get();

    auto& metrics_manager = GetForDatabase(db);
    auto metrics = metrics_manager.GetDebugMetrics();

    auto json_str = metrics.dump();

    result.SetVectorType(duckdb::VectorType::CONSTANT_VECTOR);
    auto result_data = duckdb::ConstantVector::GetData<duckdb::string_t>(result);
    result_data[0] = duckdb::StringVector::AddString(result, json_str);
}

void MetricsManager::ExecuteResetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto& context = state.GetContext();
    auto* db = context.db.get();

    auto& metrics_manager = GetForDatabase(db);
    metrics_manager.Reset();

    result.SetVectorType(duckdb::VectorType::CONSTANT_VECTOR);
    auto result_data = duckdb::ConstantVector::GetData<duckdb::string_t>(result);
    result_data[0] = duckdb::StringVector::AddString(result, "Metrics reset successfully");
}

void MetricsManager::MergeAggregateMetrics(duckdb::DatabaseInstance* db,
                                           const std::vector<const void*>& processed_state_ids,
                                           FunctionType function_type,
                                           const std::string& model_name,
                                           const std::string& provider) {
    if (processed_state_ids.empty() || db == nullptr) {
        return;
    }

    auto& manager = GetForDatabase(db);

    // Use the first state_id as the merged state_id
    const void* merged_state_id = processed_state_ids[0];

    // Start a new invocation for the merged metrics (registers the state and sets registration order)
    StartInvocation(db, merged_state_id, function_type);

    // Get and merge metrics from all processed states
    int64_t total_input_tokens = 0;
    int64_t total_output_tokens = 0;
    int64_t total_api_calls = 0;
    int64_t total_api_duration_us = 0;
    int64_t total_execution_time_us = 0;
    std::string final_model_name = model_name;
    std::string final_provider = provider;

    for (const void* state_id: processed_state_ids) {
        auto& thread_metrics = manager.GetThreadMetrics(state_id);
        const auto& metrics = thread_metrics.GetMetrics(function_type);

        if (!metrics.IsEmpty()) {
            total_input_tokens += metrics.input_tokens;
            total_output_tokens += metrics.output_tokens;
            total_api_calls += metrics.api_calls;
            total_api_duration_us += metrics.api_duration_us;
            total_execution_time_us += metrics.execution_time_us;

            // Use model info from first non-empty state if not provided
            if (final_model_name.empty() && !metrics.model_name.empty()) {
                final_model_name = metrics.model_name;
                final_provider = metrics.provider;
            }
        }
    }

    // Get the merged state's metrics and set aggregated values
    auto& merged_thread_metrics = manager.GetThreadMetrics(merged_state_id);
    auto& merged_metrics = merged_thread_metrics.GetMetrics(function_type);

    // Set the aggregated values directly
    merged_metrics.input_tokens = total_input_tokens;
    merged_metrics.output_tokens = total_output_tokens;
    merged_metrics.api_calls = total_api_calls;
    merged_metrics.api_duration_us = total_api_duration_us;
    merged_metrics.execution_time_us = total_execution_time_us;
    if (!final_model_name.empty()) {
        merged_metrics.model_name = final_model_name;
        merged_metrics.provider = final_provider;
    }

    // Clean up individual state metrics (reset function_type metrics for all except the merged one)
    for (size_t i = 1; i < processed_state_ids.size(); i++) {
        const void* state_id = processed_state_ids[i];
        auto& thread_metrics = manager.GetThreadMetrics(state_id);
        auto& metrics = thread_metrics.GetMetrics(function_type);
        // Reset only the specific function_type metrics for this state
        metrics = FunctionMetricsData{};
    }
}

}// namespace flock
