#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/fusion_combsum.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterFusionCombSUM(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("fusion_combsum", {}, duckdb::LogicalType::DOUBLE, FusionCombSUM::Execute, nullptr,
                                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

}// namespace flockmtl
