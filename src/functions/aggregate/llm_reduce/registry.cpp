#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmReduce(duckdb::DatabaseInstance& db) {
    auto string_concat = duckdb::AggregateFunction(
            "llm_reduce", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmReduce::Initialize, LlmReduce::Operation, LlmReduce::Combine,
            LlmReduce::Finalize<AggregateFunctionType::REDUCE>, LlmReduce::SimpleUpdate,
            nullptr, LlmReduce::Destroy);

    duckdb::ExtensionUtil::RegisterFunction(db, string_concat);
}

}// namespace flockmtl