#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/fusion_combmed.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterFusionCombMED(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("fusion_combmed", {}, duckdb::LogicalType::DOUBLE, FusionCombMED::Execute, nullptr,
                                                   nullptr, nullptr, nullptr, duckdb::LogicalType::ANY,
                                                   duckdb::FunctionStability::VOLATILE, duckdb::FunctionNullHandling::SPECIAL_HANDLING));
}

}// namespace flockmtl
