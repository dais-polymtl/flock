#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/fusion_rrf.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterFusionRRF(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("fusion_rrf", {}, duckdb::LogicalType::DOUBLE, FusionRRF::Execute, nullptr,
                                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

}// namespace flockmtl
