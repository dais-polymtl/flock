#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

nlohmann::json LlmFirstOrLast::Evaluate(nlohmann::json& tuples) {
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples.size()));
    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    std::vector<nlohmann::json> current_tuples = tuples;

    do {
        auto start_index = 0;
        const auto n = static_cast<int>(current_tuples.size());
        while (start_index < n) {
            auto this_batch_size = std::min<int>(batch_size, n - start_index);
            nlohmann::json batch = nlohmann::json::array();
            for (int i = 0; i < this_batch_size; ++i) {
                batch.push_back(current_tuples[start_index + i]);
            }
            const auto prompt = PromptManager::Render(user_query, batch, function_type, model.GetModelDetails().tuple_format);
            model.AddCompletionRequest(prompt, true, OutputType::INTEGER);
            start_index += this_batch_size;
        }
        std::vector<nlohmann::json> new_tuples;
        auto responses = model.CollectCompletions();
        for (size_t i = 0; i < responses.size(); ++i) {
            int result_idx = responses[i]["items"][0];
            new_tuples.push_back(current_tuples[result_idx]);
        }
        current_tuples = std::move(new_tuples);
    } while (current_tuples.size() > 1);
    current_tuples[0].erase("flockmtl_tuple_id");
    return current_tuples[0];
}

void LlmFirstOrLast::FinalizeResults(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data,
                                     duckdb::Vector& result, idx_t count, idx_t offset,
                                     AggregateFunctionType function_type) {
    const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));
    auto function_instance = AggregateFunctionBase::GetInstance<LlmFirstOrLast>();
    function_instance->function_type = function_type;

    for (idx_t i = 0; i < count; i++) {
        auto idx = i + offset;
        auto* state = states_vector[idx];

        if (state && state->value) {
            auto tuples_with_ids = nlohmann::json::array();
            for (auto j = 0; j < static_cast<int>(state->value->size()); j++) {
                auto tuple_with_id = (*state->value)[j];
                tuple_with_id["flockmtl_tuple_id"] = j;
                tuples_with_ids.push_back(tuple_with_id);
            }
            auto response = function_instance->Evaluate(tuples_with_ids);
            result.SetValue(idx, response.dump());
        } else {
            result.SetValue(idx, "{}");// Empty JSON object for null/empty states
        }
    }
}

}// namespace flockmtl
