#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

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

    return batch_tuples[0];
}

void LlmFirstOrLast::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                     duckdb::Vector& result, idx_t count, idx_t offset,
                                     AggregateFunctionType function_type) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && !state->value->empty()) {
            auto tuples_with_ids = *state->value;
            tuples_with_ids.push_back(nlohmann::json::object());
            for (auto j = 0; j < static_cast<int>((*state->value)[0]["data"].size()); j++) {
                if (j == 0) {
                    tuples_with_ids.back()["name"] = "flockmtl_row_id";
                    tuples_with_ids.back()["data"] = nlohmann::json::array();
                }
                tuples_with_ids.back()["data"].push_back(std::to_string(j));
            }
            LlmFirstOrLast function_instance;
            function_instance.function_type = function_type;
            auto response = function_instance.Evaluate(tuples_with_ids);
            result.SetValue(idx, response.dump());
        } else {
            result.SetValue(idx, nullptr);// Empty JSON object for null/empty states
        }
    }
}

}// namespace flockmtl
