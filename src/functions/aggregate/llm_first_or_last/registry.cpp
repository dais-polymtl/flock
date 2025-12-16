#include "flock/registry/registry.hpp"
#include "flock/functions/aggregate/llm_first_or_last.hpp"

namespace flock {

void AggregateRegistry::RegisterLlmFirst(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::AggregateFunction(
            "llm_first", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
            LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST>, LlmFirstOrLast::SimpleUpdate,
            LlmFirstOrLast::Bind, LlmFirstOrLast::Destroy));
}

void AggregateRegistry::RegisterLlmLast(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::AggregateFunction(
            "llm_last", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
            LlmFirstOrLast::Finalize<AggregateFunctionType::LAST>, LlmFirstOrLast::SimpleUpdate,
            LlmFirstOrLast::Bind, LlmFirstOrLast::Destroy));
}

}// namespace flock