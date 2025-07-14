#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {

nlohmann::json LlmReduce::ReduceBatch(const nlohmann::json& tuples, const AggregateFunctionType& function_type) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_query, tuples, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    model.AddCompletionRequest(prompt, true, output_type);
    auto response = model.CollectCompletions()[0];
    return response["items"][0];
};

template<>
nlohmann::json LlmReduce::ReduceLoop<ExecutionMode::SYNC>(const std::vector<nlohmann::json>& tuples,
                                                          const AggregateFunctionType& function_type) {
    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples.size()));

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

template<>
nlohmann::json LlmReduce::ReduceLoop<ExecutionMode::ASYNC>(const std::vector<nlohmann::json>& tuples,
                                                           const AggregateFunctionType& function_type) {

    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples.size()));
    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    std::vector<nlohmann::json> current_tuples = tuples;

    do {
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
    } while (current_tuples.size() > 1);

    return current_tuples[0];
}

void LlmReduce::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                duckdb::Vector& result, idx_t count, idx_t offset,
                                const AggregateFunctionType function_type, ExecutionMode mode) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    auto function_instance = AggregateFunctionBase::GetInstance<LlmReduce>();
    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && state->value) {
            nlohmann::json response;
            switch (mode) {
                case ExecutionMode::SYNC:
                    response = function_instance->ReduceLoop<ExecutionMode::ASYNC>(*state->value, function_type);
                    break;
                case ExecutionMode::ASYNC:
                    response = function_instance->ReduceLoop<ExecutionMode::SYNC>(*state->value, function_type);
                    break;
                default:
                    break;
            }
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
