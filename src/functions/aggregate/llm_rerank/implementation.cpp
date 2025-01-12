#include "flockmtl/functions/aggregate/llm_rerank.hpp"

namespace flockmtl {

int LlmRerank::GetAvailableTokens() {
    int num_tokens_meta_and_reduce_query = 0;
    num_tokens_meta_and_reduce_query += Tiktoken::GetNumTokens(user_query);
    num_tokens_meta_and_reduce_query +=
        Tiktoken::GetNumTokens(PromptManager::GetTemplate(AggregateFunctionType::RERANK));

    auto model_context_size = model.GetModelDetails().context_window;
    if (num_tokens_meta_and_reduce_query > model_context_size) {
        throw std::runtime_error("Fixed tokens exceed model context size");
    }

    auto available_tokens = model_context_size - num_tokens_meta_and_reduce_query;
    return available_tokens;
}

std::vector<int> LlmRerank::RerankBatch(const nlohmann::json& tuples) {
    nlohmann::json data;
    auto prompt =
        PromptManager::Render(user_query, tuples, AggregateFunctionType::RERANK, model.GetModelDetails().tuple_format);
    auto response = model.CallComplete(prompt);
    return response["ranking"].get<std::vector<int>>();
};

nlohmann::json LlmRerank::SlidingWindow(nlohmann::json& tuples) {
    int num_tuples = tuples.size();
    auto available_tokens = GetAvailableTokens();
    auto accumulated_rows_tokens = 0u;
    auto batch_size = 0u;
    auto window_tuples = nlohmann::json::array();
    auto start_index = num_tuples - 1;
    auto half_batch = 0;
    auto next_tuples = nlohmann::json::array();

    do {
        window_tuples.clear();
        window_tuples = std::move(next_tuples);
        next_tuples.clear();
        batch_size = half_batch;
        accumulated_rows_tokens = Tiktoken::GetNumTokens(window_tuples.dump());
        accumulated_rows_tokens +=
            Tiktoken::GetNumTokens(PromptManager::ConstructNumTuples(static_cast<int>(tuples.size())));
        accumulated_rows_tokens +=
            Tiktoken::GetNumTokens(PromptManager::ConstructInputTuplesHeader(tuples[start_index]));
        while (available_tokens - accumulated_rows_tokens > 0 && start_index >= 0) {
            auto num_tokens = Tiktoken::GetNumTokens(PromptManager::ConstructSingleInputTuple(tuples[start_index]));
            if (accumulated_rows_tokens + num_tokens > static_cast<unsigned int>(available_tokens)) {
                break;
            }
            window_tuples.push_back(tuples[start_index]);
            accumulated_rows_tokens += num_tokens;
            batch_size++;
            start_index--;
        }

        auto indexed_tuples = nlohmann::json::array();
        for (auto i = 0; i < static_cast<int>(window_tuples.size()); i++) {
            auto indexed_tuple = window_tuples[i];
            indexed_tuple["flockmtl_tuple_id"] = i;
            indexed_tuples.push_back(indexed_tuple);
        }

        auto ranked_indices = RerankBatch(indexed_tuples);

        half_batch = batch_size / 2;
        next_tuples = nlohmann::json::array();
        for (auto i = 0; i < half_batch; i++) {
            next_tuples.push_back(window_tuples[ranked_indices[i]]);
        }
    } while (start_index >= 0);

    return next_tuples;
}

void LlmRerank::Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset) {
    auto states_vector = duckdb::FlatVector::GetData<AggregateFunctionState*>(states);
    auto function_instance = AggregateFunctionBase::GetInstance<LlmRerank>();
    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto state_ptr = states_vector[idx];
        auto state = function_instance->state_map[state_ptr];

        auto tuples_with_ids = nlohmann::json::array();
        for (auto j = 0; j < static_cast<int>(state->value.size()); j++) {
            tuples_with_ids.push_back(state->value[j]);
        }
        auto reranked_tuples = function_instance->SlidingWindow(tuples_with_ids);
        result.SetValue(idx, reranked_tuples.dump());
    }
}

} // namespace flockmtl
