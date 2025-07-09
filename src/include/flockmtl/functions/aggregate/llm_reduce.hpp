#pragma once

#include "flockmtl/functions/aggregate/aggregate.hpp"

namespace flockmtl {

class LlmReduce : public AggregateFunctionBase {
public:
    explicit LlmReduce() = default;

    nlohmann::json ReduceBatch(const nlohmann::json& tuples, const AggregateFunctionType& function_type);
    nlohmann::json ReduceLoop(const std::vector<nlohmann::json>& tuples, const AggregateFunctionType& function_type);

public:
    static void Initialize(const duckdb::AggregateFunction& function, duckdb::data_ptr_t state_p) {
        AggregateFunctionBase::Initialize<LlmReduce>(function, state_p);
    }
    static void Operation(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                          duckdb::Vector& states, idx_t count) {
        AggregateFunctionBase::Operation<LlmReduce>(inputs, aggr_input_data, input_count, states, count);
    }
    static void SimpleUpdate(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                             duckdb::data_ptr_t state_p, idx_t count) {
        AggregateFunctionBase::SimpleUpdate<LlmReduce>(inputs, aggr_input_data, input_count, state_p, count);
    }
    static void Combine(duckdb::Vector& source, duckdb::Vector& target, duckdb::AggregateInputData& aggr_input_data,
                        idx_t count) {
        AggregateFunctionBase::Combine<LlmReduce>(source, target, aggr_input_data, count);
    }
    static void Destroy(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, idx_t count) {
        AggregateFunctionBase::Destroy<LlmReduce>(states, aggr_input_data, count);
    }
    static void FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                duckdb::Vector& result, idx_t count, idx_t offset,
                                const AggregateFunctionType function_type);
    template<AggregateFunctionType function_type>
    static void Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset) {
        FinalizeResults(states, aggr_input_data, result, count, offset, function_type);
    };
};

}// namespace flockmtl
