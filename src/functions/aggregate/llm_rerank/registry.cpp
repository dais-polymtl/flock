#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/aggregate/llm_rerank.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmRerank(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_rerank", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmRerank::Initialize, LlmRerank::Operation, LlmRerank::Combine, LlmRerank::Finalize, LlmRerank::SimpleUpdate,
                                                        nullptr, LlmRerank::Destroy));
}

}// namespace flockmtl
