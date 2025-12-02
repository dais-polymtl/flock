#include "flock/registry/registry.hpp"
#include "flock/metrics/metrics.hpp"

namespace flock {

void ScalarRegistry::RegisterFlockGetMetrics(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction(
            "flock_get_metrics",
            {},
            duckdb::LogicalType::JSON(),
            FlockMetrics::ExecuteGetMetrics));
}

void ScalarRegistry::RegisterFlockResetMetrics(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction(
            "flock_reset_metrics",
            {},
            duckdb::LogicalType::VARCHAR,
            FlockMetrics::ExecuteResetMetrics));
}

}// namespace flock
