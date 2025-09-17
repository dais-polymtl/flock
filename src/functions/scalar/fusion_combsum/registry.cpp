#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/fusion_combsum.hpp"

namespace flock {

void ScalarRegistry::RegisterFusionCombSUM(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("fusion_combsum", {}, duckdb::LogicalType::DOUBLE, FusionCombSUM::Execute, nullptr,
                                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

}// namespace flock
