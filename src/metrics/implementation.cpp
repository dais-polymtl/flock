#include "flock/metrics/metrics.hpp"

namespace flock {

void FlockMetrics::ExecuteGetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto json_str = GetInstance().GetMetrics().dump();
    result.SetValue(0, duckdb::Value(json_str));
}

void FlockMetrics::ExecuteResetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    GetInstance().Reset();
    result.SetValue(0, duckdb::Value("Metrics reset successfully"));
}

}// namespace flock
