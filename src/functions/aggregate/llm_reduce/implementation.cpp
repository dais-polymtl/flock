#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {


nlohmann::json LlmReduce::ReduceLoop(const std::vector<nlohmann::json>& tuples,
                                     const AggregateFunctionType& function_type) {

    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples.size()));
    if (batch_size <= 1) {
        throw std::runtime_error("Batch size must be greater than one");
    }

    std::vector<nlohmann::json> current_tuples = tuples;

    while (current_tuples.size() > 1) {
        auto start_index = 0;
        const auto n = static_cast<int>(current_tuples.size());

        // Prepare all batches and add all completion requests
        while (start_index < n) {
            auto this_batch_size = std::min<int>(batch_size, n - start_index);
            nlohmann::json batch = nlohmann::json::array();
            for (int i = 0; i < this_batch_size; ++i) {
                batch.push_back(current_tuples[start_index + i]);
            }
            const auto prompt = PromptManager::Render(user_query, batch, function_type, model.GetModelDetails().tuple_format);
            OutputType output_type = OutputType::STRING;
            model.AddCompletionRequest(prompt, true, output_type);
            start_index += this_batch_size;
        }

        // Collect all completions at once
        std::vector<nlohmann::json> new_tuples;
        auto responses = model.CollectCompletions();
        for (size_t i = 0; i < responses.size(); ++i) {
            new_tuples.push_back(responses[i]["items"][0]);
        }
        current_tuples = std::move(new_tuples);
    }
    return current_tuples[0];
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
