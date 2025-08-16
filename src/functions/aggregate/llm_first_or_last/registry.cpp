#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmFirst(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_first", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
                                                        LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST>, LlmFirstOrLast::SimpleUpdate,
                                                        nullptr, LlmFirstOrLast::Destroy));
}

void AggregateRegistry::RegisterLlmLast(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_last", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
                                                        LlmFirstOrLast::Finalize<AggregateFunctionType::LAST>, LlmFirstOrLast::SimpleUpdate,
                                                        nullptr, LlmFirstOrLast::Destroy));
}

}// namespace flockmtl