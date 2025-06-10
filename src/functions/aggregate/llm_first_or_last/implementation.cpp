#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"

namespace flockmtl {

int LlmFirstOrLast::GetAvailableTokens() {
    int num_tokens_meta_and_user_query = 0;
    num_tokens_meta_and_user_query += Tiktoken::GetNumTokens(user_query);
    num_tokens_meta_and_user_query += Tiktoken::GetNumTokens(PromptManager::GetTemplate(function_type));

    auto model_context_size = model.GetModelDetails().context_window;
    if (num_tokens_meta_and_user_query > model_context_size) {
        throw std::runtime_error("Fixed tokens exceed model context size");
    }

    auto available_tokens = model_context_size - num_tokens_meta_and_user_query;
    return available_tokens;
}

int LlmFirstOrLast::GetFirstOrLastTupleId(const nlohmann::json& tuples) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_query, tuples, function_type, model.GetModelDetails().tuple_format);
    auto response = model.CallComplete(prompt);
    return response["selected"].get<int>();
}

nlohmann::json LlmFirstOrLast::Evaluate(nlohmann::json& tuples) {
    auto available_tokens = GetAvailableTokens();
    auto accumulated_tuples_tokens = 0u;
    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;

    do {
        accumulated_tuples_tokens = Tiktoken::GetNumTokens(batch_tuples.dump());
        accumulated_tuples_tokens +=
                Tiktoken::GetNumTokens(PromptManager::ConstructNumTuples(static_cast<int>(tuples.size())));
        accumulated_tuples_tokens +=
                Tiktoken::GetNumTokens(PromptManager::ConstructInputTuplesHeader(tuples));
        while (accumulated_tuples_tokens < static_cast<unsigned int>(available_tokens) &&
               start_index < static_cast<int>(tuples.size())) {
            const auto num_tokens =
                    Tiktoken::GetNumTokens(PromptManager::ConstructSingleInputTuple(tuples[start_index]));
            if (accumulated_tuples_tokens + num_tokens > static_cast<unsigned int>(available_tokens)) {
                break;
            }
            batch_tuples.push_back(tuples[start_index]);
            accumulated_tuples_tokens += num_tokens;
            start_index++;
        }
        auto result_idx = GetFirstOrLastTupleId(batch_tuples);
        batch_tuples.clear();
        batch_tuples.push_back(tuples[result_idx]);
    } while (start_index < static_cast<int>(tuples.size()));
    batch_tuples[0].erase("flockmtl_tuple_id");

    return batch_tuples[0];
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
