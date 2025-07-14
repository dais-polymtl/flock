#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

void AggregateRegistry::RegisterLlmFirst(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_first", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
                                                        LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST, ExecutionMode::ASYNC>, LlmFirstOrLast::SimpleUpdate,
                                                        nullptr, LlmFirstOrLast::Destroy));
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_first_s", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
                                                        LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST, ExecutionMode::SYNC>, LlmFirstOrLast::SimpleUpdate,
                                                        nullptr, LlmFirstOrLast::Destroy));
}

void AggregateRegistry::RegisterLlmLast(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_last", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
                                                        LlmFirstOrLast::Finalize<AggregateFunctionType::LAST, ExecutionMode::ASYNC>, LlmFirstOrLast::SimpleUpdate,
                                                        nullptr, LlmFirstOrLast::Destroy));
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::AggregateFunction(
                                                        "llm_last_s", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                        duckdb::LogicalType::VARCHAR, duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
                                                        LlmFirstOrLast::Initialize, LlmFirstOrLast::Operation, LlmFirstOrLast::Combine,
                                                        LlmFirstOrLast::Finalize<AggregateFunctionType::LAST, ExecutionMode::SYNC>, LlmFirstOrLast::SimpleUpdate,
                                                        nullptr, LlmFirstOrLast::Destroy));
}

}// namespace flockmtl