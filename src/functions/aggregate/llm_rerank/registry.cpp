#include "flockmtl/functions/aggregate/llm_rerank.hpp"
#include "flockmtl/registry/registry.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmRerank(duckdb::DatabaseInstance& db) {
    auto string_concat = duckdb::AggregateFunction(
        "llm_rerank", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
        duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
        LlmRerank::Initialize, LlmRerank::Operation, LlmRerank::Combine, LlmRerank::Finalize, LlmRerank::SimpleUpdate);

    duckdb::ExtensionUtil::RegisterFunction(db, string_concat);
}

} // namespace flockmtl
