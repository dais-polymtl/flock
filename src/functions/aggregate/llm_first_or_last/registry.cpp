#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmFirst(duckdb::DatabaseInstance& db) {
    auto string_concat = duckdb::AggregateFunction(
            "llm_first", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
            LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST>, LlmFirstOrLast::SimpleUpdate,
            nullptr, LlmFirstOrLast::Destroy);

    duckdb::ExtensionUtil::RegisterFunction(db, string_concat);
}

void AggregateRegistry::RegisterLlmLast(duckdb::DatabaseInstance& db) {
    auto string_concat = duckdb::AggregateFunction(
            "llm_last", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
            LlmFirstOrLast::Finalize<AggregateFunctionType::LAST>, LlmFirstOrLast::SimpleUpdate,
            nullptr, LlmFirstOrLast::Destroy);

    duckdb::ExtensionUtil::RegisterFunction(db, string_concat);
}

}// namespace flockmtl