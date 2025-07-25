#include "flockmtl/functions/aggregate/llm_rerank.hpp"

namespace flockmtl {

std::vector<int> LlmRerank::RerankBatch(const nlohmann::json& tuples) {
    nlohmann::json data;
    auto prompt =
            PromptManager::Render(user_query, tuples, AggregateFunctionType::RERANK, model.GetModelDetails().tuple_format);
    model.AddCompletionRequest(prompt, static_cast<int>(tuples.size()), OutputType::INTEGER);
    auto responses = model.CollectCompletions();
    return responses[0]["items"];
};

nlohmann::json LlmRerank::SlidingWindow(nlohmann::json& tuples) {
    const auto num_tuples = static_cast<int>(tuples.size());
    auto final_ranked_tuples = nlohmann::json::array();
    auto carry_forward_tuples = nlohmann::json::array();
    auto start_index = 0;
    model = Model(model_details);

    auto batch_size = static_cast<int>(model.GetModelDetails().batch_size);
    if (batch_size == 2048) {
        batch_size = std::min<int>(batch_size, num_tuples);
    }

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    while (start_index < num_tuples || !carry_forward_tuples.empty()) {
        auto window_tuples = nlohmann::json::array();

        // First add carry forward tuples from previous batch
        for (const auto& tuple: carry_forward_tuples) {
            window_tuples.push_back(tuple);
        }

        // Then add new tuples up to batch_size
        auto remaining_space = batch_size - static_cast<int>(carry_forward_tuples.size());
        auto end_index = std::min(start_index + remaining_space, num_tuples);
        for (auto i = start_index; i < end_index; i++) {
            window_tuples.push_back(tuples[i]);
        }

        // Clear carry forward for next iteration
        carry_forward_tuples.clear();

        try {
            auto indexed_tuples = nlohmann::json::array();
            for (auto i = 0; i < static_cast<int>(window_tuples.size()); i++) {
                auto indexed_tuple = window_tuples[i];
                indexed_tuple["flockmtl_tuple_id"] = i;
                indexed_tuples.push_back(indexed_tuple);
            }

            auto ranked_indices = RerankBatch(indexed_tuples);

            // Add the bottom half to final results (they won't be re-ranked)
            auto half_batch = static_cast<int>(ranked_indices.size()) / 2;
            for (auto i = half_batch; i < static_cast<int>(ranked_indices.size()); i++) {
                auto ranked_tuple = window_tuples[ranked_indices[i]];
                if (ranked_tuple.contains("flockmtl_tuple_id")) {
                    ranked_tuple.erase("flockmtl_tuple_id");
                }
                final_ranked_tuples.push_back(ranked_tuple);
            }

            // Carry forward top half to next batch for re-ranking
            for (auto i = 0; i < half_batch; i++) {
                auto carry_tuple = window_tuples[ranked_indices[i]];
                if (carry_tuple.contains("flockmtl_tuple_id")) {
                    carry_tuple.erase("flockmtl_tuple_id");
                }
                carry_forward_tuples.push_back(carry_tuple);
            }

            start_index = end_index;

            // If we've processed all input tuples, add remaining carry forward to final results
            if (start_index >= num_tuples && !carry_forward_tuples.empty()) {
                for (const auto& tuple: carry_forward_tuples) {
                    final_ranked_tuples.insert(final_ranked_tuples.begin(), tuple);
                }
                carry_forward_tuples.clear();
            }

        } catch (const ExceededMaxOutputTokensError&) {
            // Retry the current batch with reduced size
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }
    }

    return final_ranked_tuples;
}

void LlmRerank::Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && !state->value->empty()) {
            auto tuples_with_ids = nlohmann::json::array();
            for (auto j = 0; j < static_cast<int>(state->value->size()); j++) {
                tuples_with_ids.push_back((*state->value)[j]);
            }
            LlmRerank function_instance;
            auto reranked_tuples = function_instance.SlidingWindow(tuples_with_ids);
            result.SetValue(idx, reranked_tuples.dump());
        } else {
            result.SetValue(idx, nullptr);// Empty result for null/empty states
        }
    }
}

}// namespace flockmtl
