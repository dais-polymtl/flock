#include "flock/core/config.hpp"
#include "flock/functions/aggregate/llm_first_or_last.hpp"
#include "flock/metrics/manager.hpp"

#include <chrono>
#include <vector>

namespace flock {

int LlmFirstOrLast::GetFirstOrLastTupleId(nlohmann::json& tuples) {
    nlohmann::json data;
    const auto [prompt, media_data] = PromptManager::Render(user_query, tuples, function_type, model.GetModelDetails().tuple_format);
    model.AddCompletionRequest(prompt, 1, OutputType::INTEGER, media_data);
    auto response = model.CollectCompletions()[0];
    return response["items"][0];
}

nlohmann::json LlmFirstOrLast::Evaluate(nlohmann::json& tuples) {
    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;
    model = Model(model_details);
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples[0]["data"].size()));

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    do {

        for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
            if (start_index == 0) {
                batch_tuples.push_back(nlohmann::json::object());
            }
            for (const auto& item: tuples[i].items()) {
                if (item.key() == "data") {
                    for (auto j = 0; j < batch_size && start_index + j < static_cast<int>(item.value().size()); j++) {
                        if (start_index == 0 && j == 0) {
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
            auto result_idx = GetFirstOrLastTupleId(batch_tuples);

            batch_tuples.clear();
            for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
                batch_tuples.push_back(nlohmann::json::object());
                for (const auto& item: tuples[i].items()) {
                    if (item.key() == "data") {
                        batch_tuples[i]["data"] = nlohmann::json::array();
                        batch_tuples[i]["data"].push_back(item.value()[result_idx]);
                    } else {
                        batch_tuples[i][item.key()] = item.value();
                    }
                }
            }
        } catch (const ExceededMaxOutputTokensError&) {
            start_index -= batch_size;// Retry the current batch with reduced size
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }

    } while (start_index < static_cast<int>(tuples[0]["data"].size()));

    batch_tuples.erase(batch_tuples.end() - 1);

    return batch_tuples;
}

void LlmFirstOrLast::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                     duckdb::Vector& result, idx_t count, idx_t offset,
                                     AggregateFunctionType function_type) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    // Map AggregateFunctionType to FunctionType
    FunctionType metrics_function_type = (function_type == AggregateFunctionType::FIRST) ? FunctionType::LLM_FIRST : FunctionType::LLM_LAST;

    auto db = Config::db;
    std::vector<const void*> processed_state_ids;
    std::string merged_model_name;
    std::string merged_provider;

    // Process each state individually
    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && !state->value->empty()) {
            // Use model_details and user_query from the state (not static variables)
            Model model(state->model_details);
            auto model_details_obj = model.GetModelDetails();

            // Get state ID for metrics
            const void* state_id = static_cast<const void*>(state);
            processed_state_ids.push_back(state_id);

            // Start metrics tracking
            MetricsManager::StartInvocation(db, state_id, metrics_function_type);
            MetricsManager::SetModelInfo(model_details_obj.model_name, model_details_obj.provider_name);

            // Store model info for merged metrics (use first non-empty)
            if (merged_model_name.empty() && !model_details_obj.model_name.empty()) {
                merged_model_name = model_details_obj.model_name;
                merged_provider = model_details_obj.provider_name;
            }

            auto exec_start = std::chrono::high_resolution_clock::now();

            auto tuples_with_ids = *state->value;
            tuples_with_ids.push_back(nlohmann::json::object());
            for (auto j = 0; j < static_cast<int>((*state->value)[0]["data"].size()); j++) {
                if (j == 0) {
                    tuples_with_ids.back()["name"] = "flock_row_id";
                    tuples_with_ids.back()["data"] = nlohmann::json::array();
                }
                tuples_with_ids.back()["data"].push_back(std::to_string(j));
            }
            LlmFirstOrLast function_instance;
            function_instance.function_type = function_type;
            function_instance.user_query = state->user_query;
            function_instance.model_details = state->model_details;
            auto response = function_instance.Evaluate(tuples_with_ids);

            auto exec_end = std::chrono::high_resolution_clock::now();
            double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
            MetricsManager::AddExecutionTime(exec_duration_ms);

            result.SetValue(idx, response.dump());
        } else {
            result.SetValue(idx, nullptr);
        }
    }

    // Merge all metrics from processed states into a single metrics entry
    MetricsManager::MergeAggregateMetrics(db, processed_state_ids, metrics_function_type,
                                          merged_model_name, merged_provider);
}

}// namespace flock
