#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {

nlohmann::json LlmReduce::ReduceBatch(const nlohmann::json& tuples, const AggregateFunctionType& function_type) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_query, tuples, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    auto response = model.CallComplete(prompt, true, output_type);
    return response["items"][0];
};

nlohmann::json LlmReduce::ReduceLoop(const std::vector<nlohmann::json>& tuples,
                                     const AggregateFunctionType& function_type) {
    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;
    auto batch_size = model.GetModelDetails().batch_size;

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    do {
        batch_tuples.clear();

        for (auto i = 0; i < batch_size && start_index + i < static_cast<int>(tuples.size()); i++) {
            batch_tuples.push_back(tuples[start_index + i]);
        }

        start_index += batch_size;

        try {
            auto response = ReduceBatch(batch_tuples, function_type);
            batch_tuples.clear();
            batch_tuples.push_back(response);
        } catch (const ExceededMaxOutputTokensError&) {
            start_index -= batch_size;// Retry the current batch with reduced size
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }

    } while (start_index < static_cast<int>(tuples.size()));

    return batch_tuples[0];
}

void LlmReduce::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                duckdb::Vector& result, idx_t count, idx_t offset,
                                const AggregateFunctionType function_type) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    auto function_instance = AggregateFunctionBase::GetInstance<LlmReduce>();
    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && state->value) {
            auto response = function_instance->ReduceLoop(*state->value, function_type);
            if (response.is_string()) {
                result.SetValue(idx, response.get<std::string>());
            } else {
                result.SetValue(idx, response.dump());
            }
        } else {
            result.SetValue(idx, "");// Empty result for null/empty states
        }
    }
}

}// namespace flockmtl
