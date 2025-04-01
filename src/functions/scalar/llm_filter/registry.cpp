#include "flockmtl/functions/scalar/llm_filter.hpp"
#include "flockmtl/registry/registry.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterLlmFilter(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(
        db, duckdb::ScalarFunction("llm_filter",
                                   {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                   duckdb::LogicalType::VARCHAR, LlmFilter::Execute));
}

} // namespace flockmtl
