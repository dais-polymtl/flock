#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

template void AggregateFunctionBase::Initialize<LlmFirstOrLast>(const duckdb::AggregateFunction& function,
                                                                duckdb::data_ptr_t state_p);
template void AggregateFunctionBase::Operation<LlmFirstOrLast>(duckdb::Vector[], duckdb::AggregateInputData&, idx_t,
                                                               duckdb::Vector&, idx_t);
template void AggregateFunctionBase::SimpleUpdate<LlmFirstOrLast>(duckdb::Vector[], duckdb::AggregateInputData&, idx_t,
                                                                  duckdb::data_ptr_t, idx_t);
template void AggregateFunctionBase::Combine<LlmFirstOrLast>(duckdb::Vector&, duckdb::Vector&,
                                                             duckdb::AggregateInputData&, idx_t);
template void LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST, ExecutionMode::ASYNC>(duckdb::Vector&, duckdb::AggregateInputData&,
                                                                                           duckdb::Vector&, idx_t, idx_t);
template void LlmFirstOrLast::Finalize<AggregateFunctionType::FIRST, ExecutionMode::SYNC>(duckdb::Vector&, duckdb::AggregateInputData&,
                                                                                          duckdb::Vector&, idx_t, idx_t);
template void LlmFirstOrLast::Finalize<AggregateFunctionType::LAST, ExecutionMode::ASYNC>(duckdb::Vector&, duckdb::AggregateInputData&,
                                                                                          duckdb::Vector&, idx_t, idx_t);
template void LlmFirstOrLast::Finalize<AggregateFunctionType::LAST, ExecutionMode::SYNC>(duckdb::Vector&, duckdb::AggregateInputData&,
                                                                                         duckdb::Vector&, idx_t, idx_t);

}// namespace flockmtl
