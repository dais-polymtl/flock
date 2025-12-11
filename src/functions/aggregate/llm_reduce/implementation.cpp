#include "flock/core/config.hpp"
#include "flock/functions/aggregate/llm_reduce.hpp"
#include "flock/metrics/manager.hpp"

#include <chrono>
#include <vector>

namespace flock {

nlohmann::json LlmReduce::ReduceBatch(nlohmann::json& tuples, const AggregateFunctionType& function_type, const nlohmann::json& summary) {
    auto [prompt, media_data] = PromptManager::Render(user_query, tuples, function_type, model.GetModelDetails().tuple_format);

    prompt += "\n\n" + summary.dump(4);

    OutputType output_type = OutputType::STRING;
    model.AddCompletionRequest(prompt, 1, output_type, media_data);
    auto response = model.CollectCompletions()[0];
    return response["items"][0];
};

nlohmann::json LlmReduce::ReduceLoop(const nlohmann::json& tuples,
                                     const AggregateFunctionType& function_type) {
    auto batch_tuples = nlohmann::json::array();
    auto summary = nlohmann::json::object({{"Previous Batch Summary", ""}});
    int start_index = 0;
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples[0]["data"].size()));

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    do {
        for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
            batch_tuples.push_back(nlohmann::json::object());
            for (const auto& item: tuples[i].items()) {
                if (item.key() == "data") {
                    for (auto j = 0; j < batch_size && start_index + j < static_cast<int>(item.value().size()); j++) {
                        if (j == 0) {
                            batch_tuples[i]["data"] = nlohmann::json::array();
                        }
                        batch_tuples[i]["data"].push_back(item.value()[start_index + j]);
                    }
                } else {
                    batch_tuples[i][item.key()] = item.value();
                }
            }
        }

        start_index += batch_size;

        try {
            auto response = ReduceBatch(batch_tuples, function_type, summary);
            batch_tuples.clear();
            summary = nlohmann::json::object({{"Previous Batch Summary", response}});
        } catch (const ExceededMaxOutputTokensError&) {
            start_index -= batch_size;// Retry the current batch with reduced size
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }

    } while (start_index < static_cast<int>(tuples[0]["data"].size()));

    return summary["Previous Batch Summary"];
}

void LlmReduce::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                duckdb::Vector& result, idx_t count, idx_t offset,
                                const AggregateFunctionType function_type) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    auto db = Config::db;
    std::vector<const void*> processed_state_ids;
    std::string merged_model_name;
    std::string merged_provider;

    // Process each state individually
    for (idx_t i = 0; i < count; i++) {
        auto result_idx = i + offset;
        auto* state = states_vector[i];

        if (state && state->value && !state->value->empty()) {
            // Use model_details and user_query from the state
            Model model(state->model_details);
            auto model_details_obj = model.GetModelDetails();

            // Get state ID for metrics
            const void* state_id = static_cast<const void*>(state);
            processed_state_ids.push_back(state_id);

            // Start metrics tracking for this state
            MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_REDUCE);
            MetricsManager::SetModelInfo(model_details_obj.model_name, model_details_obj.provider_name);

            // Store model info for merged metrics (use first non-empty)
            if (merged_model_name.empty() && !model_details_obj.model_name.empty()) {
                merged_model_name = model_details_obj.model_name;
                merged_provider = model_details_obj.provider_name;
            }

            auto exec_start = std::chrono::high_resolution_clock::now();

            LlmReduce reduce_instance;
            reduce_instance.model = Model(state->model_details);
            reduce_instance.user_query = state->user_query;
            auto response = reduce_instance.ReduceLoop(*state->value, function_type);

            auto exec_end = std::chrono::high_resolution_clock::now();
            double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
            MetricsManager::AddExecutionTime(exec_duration_ms);

            if (response.is_string()) {
                result.SetValue(result_idx, response.get<std::string>());
            } else {
                result.SetValue(result_idx, response.dump());
            }
        } else {
            result.SetValue(result_idx, nullptr);
        }
    }

    // Merge all metrics from processed states into a single metrics entry
    MetricsManager::MergeAggregateMetrics(db, processed_state_ids, FunctionType::LLM_REDUCE,
                                          merged_model_name, merged_provider);
}

}// namespace flock
