#include "flock/core/config.hpp"
#include "flock/functions/aggregate/llm_first_or_last.hpp"
#include "flock/functions/llm_function_bind_data.hpp"
#include "flock/metrics/manager.hpp"

#include <chrono>
#include <set>

namespace flock {

duckdb::unique_ptr<duckdb::FunctionData> LlmFirstOrLast::Bind(
        duckdb::ClientContext& context,
        duckdb::AggregateFunction& function,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments) {
    return AggregateFunctionBase::ValidateAndInitializeBindData(context, arguments, function.name);
}

int LlmFirstOrLast::GetFirstOrLastTupleId(nlohmann::json& tuples) {
    const auto [prompt, media_data] = PromptManager::Render(
            user_query, tuples, function_type, model.GetModelDetails().tuple_format);
    model.AddCompletionRequest(prompt, 1, OutputType::INTEGER, media_data);
    auto response = model.CollectCompletions()[0];

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

    int result_id_int = -1;
    std::string result_id_str;
    if (response["items"][0].is_number_integer()) {
        result_id_int = response["items"][0].get<int>();
        result_id_str = std::to_string(result_id_int);
    } else if (response["items"][0].is_string()) {
        result_id_str = response["items"][0].get<std::string>();
        try {
            result_id_int = std::stoi(result_id_str);
        } catch (...) {
            throw std::runtime_error(
                    "Invalid LLM response: The LLM returned ID '" + result_id_str +
                    "' which is not a valid flock_row_id.");
        }
    } else {
        throw std::runtime_error(
                "Invalid LLM response: Expected integer or string ID, got: " +
                response["items"][0].dump());
    }

    if (valid_ids.find(result_id_str) == valid_ids.end()) {
        throw std::runtime_error(
                "Invalid LLM response: The LLM returned ID '" + result_id_str +
                "' which is not a valid flock_row_id.");
    }

    return result_id_int;
}

nlohmann::json LlmFirstOrLast::Evaluate(nlohmann::json& tuples) {
    int num_tuples = static_cast<int>(tuples[0]["data"].size());

    if (num_tuples <= 1) {
        auto result = nlohmann::json::array();
        for (auto i = 0; i < static_cast<int>(tuples.size()) - 1; i++) {
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

    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, num_tuples);

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
            for (auto i = 0; i < static_cast<int>(tuples.size()) - 1; i++) {
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
            start_index -= batch_size;
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }

    } while (start_index < static_cast<int>(tuples[0]["data"].size()));

    return batch_tuples;
}

void LlmFirstOrLast::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                     duckdb::Vector& result, idx_t count, idx_t offset,
                                     AggregateFunctionType function_type) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(
            duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    FunctionType metrics_function_type =
            (function_type == AggregateFunctionType::FIRST) ? FunctionType::LLM_FIRST : FunctionType::LLM_LAST;

    auto& bind_data = aggr_input_data.bind_data->Cast<LlmFunctionBindData>();

    auto temp_model = bind_data.CreateModel();
    auto model_details_obj = temp_model.GetModelDetails();

    auto db = Config::db;
    std::vector<const void*> processed_state_ids;

    for (idx_t i = 0; i < count; i++) {
        auto result_idx = i + offset;
        auto* state = states_vector[i];

        if (!state || !state->value || state->value->empty()) {
            result.SetValue(result_idx, nullptr);
            continue;
        }

        int num_tuples = static_cast<int>((*state->value)[0]["data"].size());

        if (num_tuples <= 1) {
            auto response = nlohmann::json::array();
            for (auto k = 0; k < static_cast<int>(state->value->size()); k++) {
                response.push_back(nlohmann::json::object());
                for (const auto& item: (*state->value)[k].items()) {
                    if (item.key() == "data") {
                        response[k]["data"] = nlohmann::json::array();
                        if (!item.value().empty()) {
                            response[k]["data"].push_back(item.value()[0]);
                        }
                    } else {
                        response[k][item.key()] = item.value();
                    }
                }
            }
            result.SetValue(result_idx, response.dump());
            continue;
        }

        const void* state_id = static_cast<const void*>(state);
        processed_state_ids.push_back(state_id);
        MetricsManager::StartInvocation(db, state_id, metrics_function_type);
        MetricsManager::SetModelInfo(model_details_obj.model_name, model_details_obj.provider_name);

        auto exec_start = std::chrono::high_resolution_clock::now();

        nlohmann::json tuples_with_ids = *state->value;

        tuples_with_ids.push_back({{"name", "flock_row_id"}, {"data", nlohmann::json::array()}});
        for (int j = 0; j < num_tuples; j++) {
            tuples_with_ids.back()["data"].push_back(std::to_string(j));
        }

        if (bind_data.prompt.empty()) {
            throw std::runtime_error("The prompt cannot be empty");
        }

        LlmFirstOrLast function_instance;
        function_instance.function_type = function_type;
        function_instance.user_query = bind_data.prompt;
        function_instance.model = bind_data.CreateModel();
        auto response = function_instance.Evaluate(tuples_with_ids);

        auto exec_end = std::chrono::high_resolution_clock::now();
        double exec_duration_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();
        MetricsManager::AddExecutionTime(exec_duration_ms);

        result.SetValue(result_idx, response.dump());
    }

    if (!processed_state_ids.empty()) {
        MetricsManager::MergeAggregateMetrics(db, processed_state_ids, metrics_function_type,
                                              model_details_obj.model_name, model_details_obj.provider_name);
    }
}

}// namespace flock
