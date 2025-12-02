#include "flock/metrics/manager.hpp"

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

}// namespace flock
