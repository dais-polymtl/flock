#include "flock/registry/registry.hpp"
#include "flock/functions/aggregate/llm_reduce.hpp"

namespace flock {

void AggregateRegistry::RegisterLlmReduce(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::AggregateFunction(
            "llm_reduce", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmReduce::Initialize, LlmReduce::Operation, LlmReduce::Combine,
            LlmReduce::Finalize<AggregateFunctionType::REDUCE>, LlmReduce::SimpleUpdate,
            LlmReduce::Bind, LlmReduce::Destroy));
}

}// namespace flock