#include "flockmtl/functions/scalar/fusion_combmed.hpp"
#include "flockmtl/registry/registry.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterFusionCombMED(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(
        db, duckdb::ScalarFunction("fusion_combmed", {}, duckdb::LogicalType::VARCHAR, FusionCombMED::Execute, nullptr,
                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

} // namespace flockmtl
