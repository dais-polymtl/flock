#include "flockmtl/functions/aggregate/llm_rerank.hpp"

namespace flockmtl {

std::vector<int> LlmRerank::RerankBatch(const nlohmann::json& tuples) {
    nlohmann::json data;
    auto prompt =
            PromptManager::Render(user_query, tuples, AggregateFunctionType::RERANK, model.GetModelDetails().tuple_format);
    model.AddCompletionRequest(prompt, true, OutputType::INTEGER);
    auto responses = model.CollectCompletions();
    return responses[0]["items"];
};

nlohmann::json LlmRerank::SlidingWindow(nlohmann::json& tuples) {
    const auto num_tuples = static_cast<int>(tuples.size());
    auto window_tuples = nlohmann::json::array();
    auto next_tuples = nlohmann::json::array();
    auto start_index = num_tuples - 1;
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, num_tuples);

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    do {
        window_tuples.clear();
        window_tuples = std::move(next_tuples);
        next_tuples.clear();

        for (auto i = 0; i < batch_size && start_index - i >= 0; i++) {
            window_tuples.push_back(tuples[start_index - i]);
        }

        start_index -= batch_size;

        try {
            auto indexed_tuples = nlohmann::json::array();
            for (auto i = 0; i < static_cast<int>(window_tuples.size()); i++) {
                auto indexed_tuple = window_tuples[i];
                indexed_tuple["flockmtl_tuple_id"] = i;
                indexed_tuples.push_back(indexed_tuple);
            }

            auto ranked_indices = RerankBatch(indexed_tuples);
            auto half_batch = batch_size / 2 + 1;
            next_tuples = nlohmann::json::array();
            for (auto i = 0; i < half_batch; i++) {
                next_tuples.push_back(window_tuples[ranked_indices[i]]);
            }
        } catch (const ExceededMaxOutputTokensError&) {
            start_index += batch_size;// Retry the current batch with reduced size
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }

    } while (start_index >= 0);

    return next_tuples;
}

void LlmRerank::Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));
    auto function_instance = AggregateFunctionBase::GetInstance<LlmRerank>();

    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && state->value) {
            auto tuples_with_ids = nlohmann::json::array();
            for (auto j = 0; j < static_cast<int>(state->value->size()); j++) {
                tuples_with_ids.push_back((*state->value)[j]);
            }
            auto reranked_tuples = function_instance->SlidingWindow(tuples_with_ids);
            result.SetValue(idx, reranked_tuples.dump());
        } else {
            result.SetValue(idx, "[]");// Empty result for null/empty states
        }
    }
}

}// namespace flockmtl
