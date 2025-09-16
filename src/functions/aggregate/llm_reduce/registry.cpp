#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmReduce(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::AggregateFunction(
            "llm_reduce", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmReduce::Initialize, LlmReduce::Operation, LlmReduce::Combine,
            LlmReduce::Finalize<AggregateFunctionType::REDUCE>, LlmReduce::SimpleUpdate,
            nullptr, LlmReduce::Destroy));
}

}// namespace flockmtl