#include "flock/registry/registry.hpp"
#include "flock/metrics/manager.hpp"

namespace flock {

void ScalarRegistry::RegisterFlockGetMetrics(duckdb::ExtensionLoader& loader) {
    auto function = duckdb::ScalarFunction(
            "flock_get_metrics",
            {},
            duckdb::LogicalType::JSON(),
            MetricsManager::ExecuteGetMetrics);
    function.stability = duckdb::FunctionStability::VOLATILE;
    loader.RegisterFunction(function);
}

void ScalarRegistry::RegisterFlockGetDebugMetrics(duckdb::ExtensionLoader& loader) {
    auto function = duckdb::ScalarFunction(
            "flock_get_debug_metrics",
            {},
            duckdb::LogicalType::JSON(),
            MetricsManager::ExecuteGetDebugMetrics);
    function.stability = duckdb::FunctionStability::VOLATILE;
    loader.RegisterFunction(function);
}

void ScalarRegistry::RegisterFlockResetMetrics(duckdb::ExtensionLoader& loader) {
    auto function = duckdb::ScalarFunction(
            "flock_reset_metrics",
            {},
            duckdb::LogicalType::VARCHAR,
            MetricsManager::ExecuteResetMetrics);
    function.stability = duckdb::FunctionStability::VOLATILE;
    loader.RegisterFunction(function);
}

}// namespace flock
