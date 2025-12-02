#include "flock/functions/aggregate/llm_rerank.hpp"
#include "flock/metrics/metrics.hpp"

#include <chrono>

namespace flock {

std::vector<int> LlmRerank::RerankBatch(const nlohmann::json& tuples) {
    nlohmann::json data;
    auto [prompt, media_data] =
            PromptManager::Render(user_query, tuples, AggregateFunctionType::RERANK, model.GetModelDetails().tuple_format);
    model.AddCompletionRequest(prompt, static_cast<int>(tuples[0]["data"].size()), OutputType::INTEGER, media_data);
    auto responses = model.CollectCompletions();
    return responses[0]["items"];
};

nlohmann::json LlmRerank::SlidingWindow(nlohmann::json& tuples) {
    const auto num_tuples = static_cast<int>(tuples[0]["data"].size());
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
        auto window_tuples = carry_forward_tuples;

        // Then add new tuples up to batch_size
        auto remaining_space = batch_size - static_cast<int>(window_tuples[0]["data"].size());
        auto end_index = std::min<int>(start_index + remaining_space, num_tuples);
        for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
            if (i >= static_cast<int>(window_tuples.size())) {
                window_tuples.push_back(nlohmann::json::object());
            }
            for (const auto& item: tuples[i].items()) {
                if (item.key() == "data") {
                    for (auto j = start_index; j < end_index; j++) {
                        if (j == 0) {
                            window_tuples[i]["data"] = nlohmann::json::array();
                        }
                        window_tuples[i]["data"].push_back(item.value()[j]);
                    }
                } else {
                    window_tuples[i][item.key()] = item.value();
                }
            }
        }

        // Clear carry forward for next iteration
        carry_forward_tuples.clear();

        try {
            auto indexed_tuples = window_tuples;
            indexed_tuples.push_back(nlohmann::json::object());
            for (auto i = 0; i < static_cast<int>(window_tuples[0]["data"].size()); i++) {
                if (i == 0) {
                    indexed_tuples.back()["name"] = "flock_row_id";
                    indexed_tuples.back()["data"] = nlohmann::json::array();
                }
                indexed_tuples.back()["data"].push_back(std::to_string(i));
            }

            auto ranked_indices = RerankBatch(indexed_tuples);

            // Add the bottom half to final results (they won't be re-ranked)
            auto half_batch = static_cast<int>(ranked_indices.size()) / 2;
            for (auto i = half_batch; i < static_cast<int>(ranked_indices.size()); i++) {
                auto idx = 0u;
                for (auto& column: window_tuples) {
                    final_ranked_tuples[idx]["data"].push_back(column["data"][ranked_indices[i]]);
                    idx++;
                }
            }

            // Carry forward top half to next batch for re-ranking
            for (auto i = 0; i < half_batch; i++) {
                auto idx = 0u;
                for (auto& column: window_tuples) {
                    carry_forward_tuples[idx]["data"].push_back(column["data"][ranked_indices[i]]);
                    idx++;
                }
            }

            start_index = end_index;

            // If we've processed all input tuples, add remaining carry forward to final results
            if (start_index >= num_tuples && !carry_forward_tuples.empty()) {
                auto idx = 0u;
                for (const auto& column: carry_forward_tuples) {
                    for (const auto& i: column["data"]) {
                        final_ranked_tuples[idx]["data"].push_back(i);
                    }
                    idx++;
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
    // Start execution timing
    auto exec_start = std::chrono::high_resolution_clock::now();

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

    // End execution timing and update metrics
    auto exec_end = std::chrono::high_resolution_clock::now();
    double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
    FlockMetrics::GetInstance().AddExecutionTime(exec_duration_ms);
}

}// namespace flock
