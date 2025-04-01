#include "flockmtl/functions/scalar/fusion_rrf.hpp"
#include "flockmtl/registry/registry.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterFusionRRF(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(
        db, duckdb::ScalarFunction("fusion_rrf", {}, duckdb::LogicalType::DOUBLE, FusionRRF::Execute, nullptr,
                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

} // namespace flockmtl
