#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/llm_filter.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterLlmFilter(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("llm_filter",
                                                   {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                   duckdb::LogicalType::VARCHAR, LlmFilter::Execute));
}

}// namespace flockmtl
