#include "flockmtl/functions/aggregate/llm_reduce.hpp"

namespace flockmtl {

int LlmReduce::GetAvailableTokens(const AggregateFunctionType& function_type) {
    int num_tokens_meta_and_reduce_query = 0;
    num_tokens_meta_and_reduce_query += Tiktoken::GetNumTokens(user_query);
    num_tokens_meta_and_reduce_query += Tiktoken::GetNumTokens(PromptManager::GetTemplate(function_type));

    auto model_context_size = model.GetModelDetails().context_window;
    if (num_tokens_meta_and_reduce_query > model_context_size) {
        throw std::runtime_error("Fixed tokens exceed model context size");
    }

    auto available_tokens = model_context_size - num_tokens_meta_and_reduce_query;
    return available_tokens;
}

nlohmann::json LlmReduce::ReduceBatch(const nlohmann::json& tuples, const AggregateFunctionType& function_type) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_query, tuples, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    auto response = model.CallComplete(prompt, true, output_type);
    return response["items"][0];
};

nlohmann::json LlmReduce::ReduceLoop(const std::vector<nlohmann::json>& tuples,
                                     const AggregateFunctionType& function_type) {
    auto available_tokens = GetAvailableTokens(function_type);
    auto accumulated_tuples_tokens = 0u;
    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;

    do {
        accumulated_tuples_tokens = Tiktoken::GetNumTokens(batch_tuples.dump());
        accumulated_tuples_tokens +=
                Tiktoken::GetNumTokens(PromptManager::ConstructNumTuples(static_cast<int>(tuples.size())));
        accumulated_tuples_tokens +=
                Tiktoken::GetNumTokens(PromptManager::ConstructInputTuplesHeader(tuples));
        while (accumulated_tuples_tokens < static_cast<unsigned int>(available_tokens) &&
               start_index < static_cast<int>(tuples.size())) {
            const auto num_tokens =
                    Tiktoken::GetNumTokens(PromptManager::ConstructSingleInputTuple(tuples[start_index]));
            if (accumulated_tuples_tokens + num_tokens > static_cast<unsigned int>(available_tokens)) {
                break;
            }
            batch_tuples.push_back(tuples[start_index]);
            accumulated_tuples_tokens += num_tokens;
            start_index++;
        }
        auto response = ReduceBatch(batch_tuples, function_type);
        batch_tuples.clear();
        batch_tuples.push_back(response);
        accumulated_tuples_tokens = 0u;
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
