#include "flock/core/config.hpp"
#include "flock/functions/aggregate/llm_rerank.hpp"
#include "flock/functions/llm_function_bind_data.hpp"
#include "flock/metrics/manager.hpp"

#include <chrono>
#include <set>

namespace flock {

duckdb::unique_ptr<duckdb::FunctionData> LlmRerank::Bind(
        duckdb::ClientContext& context,
        duckdb::AggregateFunction& function,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments) {
    return AggregateFunctionBase::ValidateAndInitializeBindData(context, arguments, "llm_rerank");
}

std::vector<int> LlmRerank::RerankBatch(const nlohmann::json& tuples) {
    auto [prompt, media_data] = PromptManager::Render(
            user_query, tuples, AggregateFunctionType::RERANK, model.GetModelDetails().tuple_format);

    int num_tuples = static_cast<int>(tuples[0]["data"].size());

    model.AddCompletionRequest(prompt, num_tuples, OutputType::INTEGER, media_data);
    auto responses = model.CollectCompletions();

    // Find flock_row_id column to get valid IDs
    std::set<std::string> valid_ids;
    for (const auto& column: tuples) {
        if (column.contains("name") && column["name"].is_string() &&
            column["name"].get<std::string>() == "flock_row_id" &&
            column.contains("data") && column["data"].is_array()) {
            for (const auto& id: column["data"]) {
                if (id.is_string()) {
                    valid_ids.insert(id.get<std::string>());
                }
            }
            break;
        }
    }

    std::vector<int> indices;
    std::set<std::string> seen_ids;

    for (const auto& item: responses[0]["items"]) {
        std::string id_str;
        int id_int = -1;

        // Handle both integer and string responses
        if (item.is_number_integer()) {
            id_int = item.get<int>();
            id_str = std::to_string(id_int);
        } else if (item.is_string()) {
            id_str = item.get<std::string>();
            try {
                id_int = std::stoi(id_str);
            } catch (...) {
                throw std::runtime_error(
                        "Invalid LLM response: The LLM returned ID '" + id_str +
                        "' which is not a valid flock_row_id.");
            }
        } else {
            throw std::runtime_error(
                    "Invalid LLM response: Expected integer or string ID, got: " + item.dump());
        }

        // Validate that the ID exists in flock_row_id
        if (valid_ids.find(id_str) == valid_ids.end()) {
            throw std::runtime_error(
                    "Invalid LLM response: The LLM returned ID '" + id_str +
                    "' which is not a valid flock_row_id.");
        }

        // Check for duplicates
        if (seen_ids.count(id_str) > 0) {
            throw std::runtime_error(
                    "Invalid LLM response: The LLM returned duplicate ID '" + id_str + "'.");
        }
        seen_ids.insert(id_str);
        indices.push_back(id_int);
    }

    return indices;
}

nlohmann::json LlmRerank::SlidingWindow(nlohmann::json& tuples) {
    const int num_tuples = static_cast<int>(tuples[0]["data"].size());

    // If there's only 1 tuple, no need to call the LLM - just return it
    if (num_tuples <= 1) {
        auto result = nlohmann::json::array();
        for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
            result.push_back(nlohmann::json::object());
            for (const auto& item: tuples[i].items()) {
                if (item.key() == "data") {
                    result[i]["data"] = nlohmann::json::array();
                    if (!item.value().empty()) {
                        result[i]["data"].push_back(item.value()[0]);
                    }
                } else {
                    result[i][item.key()] = item.value();
                }
            }
        }
        return result;
    }

    auto final_ranked_tuples = nlohmann::json::array();
    auto carry_forward_tuples = nlohmann::json::array();
    int start_index = 0;

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
        // Handle case where carry_forward_tuples is empty (first iteration)
        int remaining_space = window_tuples.empty()
                                      ? batch_size
                                      : (batch_size - static_cast<int>(window_tuples[0]["data"].size()));
        int end_index = std::min<int>(start_index + remaining_space, num_tuples);

        for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
            if (i >= static_cast<int>(window_tuples.size())) {
                window_tuples.push_back(nlohmann::json::object());
            }
            for (const auto& item: tuples[i].items()) {
                if (item.key() == "data") {
                    if (!window_tuples[i].contains("data")) {
                        window_tuples[i]["data"] = nlohmann::json::array();
                    }
                    for (int j = start_index; j < end_index; j++) {
                        window_tuples[i]["data"].push_back(item.value()[j]);
                    }
                } else {
                    window_tuples[i][item.key()] = item.value();
                }
            }
        }

        // Clear carry forward for next iteration
        carry_forward_tuples.clear();

        // Skip if window_tuples is empty (shouldn't happen, but safety check)
        if (window_tuples.empty() || window_tuples[0]["data"].empty()) {
            continue;
        }

        try {
            // Build indexed tuples with flock_row_id
            auto indexed_tuples = window_tuples;
            indexed_tuples.push_back({{"name", "flock_row_id"}, {"data", nlohmann::json::array()}});
            for (int i = 0; i < static_cast<int>(window_tuples[0]["data"].size()); i++) {
                indexed_tuples.back()["data"].push_back(std::to_string(i));
            }

            auto ranked_indices = RerankBatch(indexed_tuples);

            // Initialize final_ranked_tuples structure if needed (first time adding results)
            if (final_ranked_tuples.empty() && !window_tuples.empty()) {
                for (size_t i = 0; i < window_tuples.size(); i++) {
                    final_ranked_tuples.push_back(nlohmann::json::object());
                    // Copy metadata from window_tuples
                    for (const auto& item: window_tuples[i].items()) {
                        if (item.key() != "data") {
                            final_ranked_tuples[i][item.key()] = item.value();
                        }
                    }
                    final_ranked_tuples[i]["data"] = nlohmann::json::array();
                }
            }

            // Add the bottom half to final results (they won't be re-ranked)
            int half_batch = static_cast<int>(ranked_indices.size()) / 2;
            for (int i = half_batch; i < static_cast<int>(ranked_indices.size()); i++) {
                size_t idx = 0;
                for (auto& column: window_tuples) {
                    final_ranked_tuples[idx]["data"].push_back(column["data"][ranked_indices[i]]);
                    idx++;
                }
            }

            // Carry forward top half to next batch for re-ranking
            // Initialize carry_forward_tuples structure if needed
            if (carry_forward_tuples.empty() && !window_tuples.empty()) {
                for (size_t i = 0; i < window_tuples.size(); i++) {
                    carry_forward_tuples.push_back(nlohmann::json::object());
                    // Copy metadata from window_tuples
                    for (const auto& item: window_tuples[i].items()) {
                        if (item.key() != "data") {
                            carry_forward_tuples[i][item.key()] = item.value();
                        }
                    }
                    carry_forward_tuples[i]["data"] = nlohmann::json::array();
                }
            }
            for (int i = 0; i < half_batch; i++) {
                size_t idx = 0;
                for (auto& column: window_tuples) {
                    carry_forward_tuples[idx]["data"].push_back(column["data"][ranked_indices[i]]);
                    idx++;
                }
            }

            start_index = end_index;

            // If we've processed all input tuples, add remaining carry forward to final results
            if (start_index >= num_tuples && !carry_forward_tuples.empty()) {
                size_t idx = 0;
                for (const auto& column: carry_forward_tuples) {
                    for (const auto& data_item: column["data"]) {
                        final_ranked_tuples[idx]["data"].push_back(data_item);
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

void LlmRerank::Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                         duckdb::Vector& result, idx_t count, idx_t offset) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(
            duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    // Get bind data - model_json and prompt are guaranteed to be initialized
    auto& bind_data = aggr_input_data.bind_data->Cast<LlmFunctionBindData>();

    // Get model details for metrics (create temp model just for details)
    auto temp_model = bind_data.CreateModel();
    auto model_details_obj = temp_model.GetModelDetails();

    auto db = Config::db;
    std::vector<const void*> processed_state_ids;

    // Process each state individually
    for (idx_t i = 0; i < count; i++) {
        auto result_idx = i + offset;
        auto* state = states_vector[i];

        if (!state || !state->value || state->value->empty()) {
            result.SetValue(result_idx, nullptr);
            continue;
        }

        // Track metrics for this state
        const void* state_id = static_cast<const void*>(state);
        processed_state_ids.push_back(state_id);
        MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_RERANK);
        MetricsManager::SetModelInfo(model_details_obj.model_name, model_details_obj.provider_name);

        auto exec_start = std::chrono::high_resolution_clock::now();

        // Copy state value to avoid potential use-after-free issues
        nlohmann::json tuples = *state->value;

        // Create function instance with bind data
        // IMPORTANT: Use CreateModel() for thread-safe Model instance
        LlmRerank function_instance;
        function_instance.user_query = bind_data.prompt;
        function_instance.model = bind_data.CreateModel();
        auto reranked_tuples = function_instance.SlidingWindow(tuples);

        auto exec_end = std::chrono::high_resolution_clock::now();
        double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
        MetricsManager::AddExecutionTime(exec_duration_ms);

        result.SetValue(result_idx, reranked_tuples.dump());
    }

    // Merge all metrics from processed states
    if (!processed_state_ids.empty()) {
        MetricsManager::MergeAggregateMetrics(db, processed_state_ids, FunctionType::LLM_RERANK,
                                              model_details_obj.model_name, model_details_obj.provider_name);
    }
}

}// namespace flock
