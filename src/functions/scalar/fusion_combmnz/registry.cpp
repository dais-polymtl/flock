#include "flockmtl/functions/scalar/fusion_combmnz.hpp"
#include "flockmtl/registry/registry.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterFusionCombMNZ(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(
        db, duckdb::ScalarFunction("fusion_combmnz", {}, duckdb::LogicalType::VARCHAR, FusionCombMNZ::Execute, nullptr,
                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

} // namespace flockmtl
