#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/fusion_combmnz.hpp"

namespace flock {

void ScalarRegistry::RegisterFusionCombMNZ(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("fusion_combmnz", {}, duckdb::LogicalType::DOUBLE, FusionCombMNZ::Execute, nullptr,
                                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

}// namespace flock
