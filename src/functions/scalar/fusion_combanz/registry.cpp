#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/fusion_combanz.hpp"

namespace flock {

void ScalarRegistry::RegisterFusionCombANZ(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("fusion_combanz", {}, duckdb::LogicalType::DOUBLE, FusionCombANZ::Execute, nullptr,
                                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

}// namespace flock
