#pragma once

#include "flock/functions/aggregate/aggregate.hpp"
#include "flock/functions/llm_function_bind_data.hpp"

namespace flock {

class LlmFirstOrLast : public AggregateFunctionBase {
public:
    AggregateFunctionType function_type;

public:
    explicit LlmFirstOrLast() = default;

    int GetFirstOrLastTupleId(nlohmann::json& tuples);
    nlohmann::json Evaluate(nlohmann::json& tuples);

    static duckdb::unique_ptr<duckdb::FunctionData> Bind(duckdb::ClientContext& context, duckdb::AggregateFunction& function, duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments);


public:
    static void Initialize(const duckdb::AggregateFunction& function, duckdb::data_ptr_t state_p) {
        AggregateFunctionBase::Initialize<LlmFirstOrLast>(function, state_p);
    }
    static void Operation(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                          duckdb::Vector& states, idx_t count) {
        AggregateFunctionBase::Operation<LlmFirstOrLast>(inputs, aggr_input_data, input_count, states, count);
    }
    static void SimpleUpdate(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                             duckdb::data_ptr_t state_p, idx_t count) {
        AggregateFunctionBase::SimpleUpdate<LlmFirstOrLast>(inputs, aggr_input_data, input_count, state_p, count);
    }
    static void Combine(duckdb::Vector& source, duckdb::Vector& target, duckdb::AggregateInputData& aggr_input_data,
                        idx_t count) {
        AggregateFunctionBase::Combine<LlmFirstOrLast>(source, target, aggr_input_data, count);
    }
    static void Destroy(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, idx_t count) {
        AggregateFunctionBase::Destroy<LlmFirstOrLast>(states, aggr_input_data, count);
    }
    template<AggregateFunctionType function_type>
    static void Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset) {
        FinalizeResults(states, aggr_input_data, result, count, offset, function_type);
    }
    static void FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                duckdb::Vector& result, idx_t count, idx_t offset, AggregateFunctionType function_type);
};

}// namespace flock
