#pragma once

#include "flock/functions/aggregate/aggregate.hpp"
#include "flock/functions/llm_function_bind_data.hpp"

namespace flock {

class LlmRerank : public AggregateFunctionBase {
public:
    explicit LlmRerank() = default;

    nlohmann::json SlidingWindow(nlohmann::json& tuples);
    std::vector<int> RerankBatch(const nlohmann::json& tuples);

public:
    static duckdb::unique_ptr<duckdb::FunctionData> Bind(
            duckdb::ClientContext& context,
            duckdb::AggregateFunction& function,
            duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments);

    static void Initialize(const duckdb::AggregateFunction& function, duckdb::data_ptr_t state_p) {
        AggregateFunctionBase::Initialize<LlmRerank>(function, state_p);
    }
    static void Operation(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                          duckdb::Vector& states, idx_t count) {
        AggregateFunctionBase::Operation<LlmRerank>(inputs, aggr_input_data, input_count, states, count);
    }
    static void SimpleUpdate(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                             duckdb::data_ptr_t state_p, idx_t count) {
        AggregateFunctionBase::SimpleUpdate<LlmRerank>(inputs, aggr_input_data, input_count, state_p, count);
    }
    static void Combine(duckdb::Vector& source, duckdb::Vector& target, duckdb::AggregateInputData& aggr_input_data,
                        idx_t count) {
        AggregateFunctionBase::Combine<LlmRerank>(source, target, aggr_input_data, count);
    }
    static void Destroy(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, idx_t count) {
        AggregateFunctionBase::Destroy<LlmRerank>(states, aggr_input_data, count);
    }
    static void Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset);
};

}// namespace flock
