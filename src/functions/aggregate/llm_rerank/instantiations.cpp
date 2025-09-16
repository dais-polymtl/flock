#include "flock/functions/aggregate/llm_rerank.hpp"

namespace flock {

template void AggregateFunctionBase::Initialize<LlmRerank>(const duckdb::AggregateFunction& function,
                                                           duckdb::data_ptr_t state_p);
template void AggregateFunctionBase::Operation<LlmRerank>(duckdb::Vector[], duckdb::AggregateInputData&, idx_t,
                                                          duckdb::Vector&, idx_t);
template void AggregateFunctionBase::SimpleUpdate<LlmRerank>(duckdb::Vector[], duckdb::AggregateInputData&, idx_t,
                                                             duckdb::data_ptr_t, idx_t);
template void AggregateFunctionBase::Combine<LlmRerank>(duckdb::Vector&, duckdb::Vector&, duckdb::AggregateInputData&,
                                                        idx_t);

}// namespace flock
