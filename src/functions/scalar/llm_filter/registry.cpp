#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/llm_filter.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterLlmFilter(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(
            db, duckdb::ScalarFunction("llm_filter",
                                       {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                       duckdb::LogicalType::VARCHAR, LlmFilter::Execute<ExecutionMode::ASYNC>));
    duckdb::ExtensionUtil::RegisterFunction(
            db, duckdb::ScalarFunction("llm_filter_s",
                                       {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                       duckdb::LogicalType::VARCHAR, LlmFilter::Execute<ExecutionMode::SYNC>));
}

}// namespace flockmtl
