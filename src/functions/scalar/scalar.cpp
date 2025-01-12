#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

nlohmann::json ScalarFunctionBase::Complete(const nlohmann::json& tuples, const std::string& user_prompt,
                                            ScalarFunctionType function_type, Model& model) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_prompt, tuples, function_type);
    auto response = model.CallComplete(prompt);
    return response["tuples"];
};

nlohmann::json ScalarFunctionBase::BatchAndComplete(const std::vector<nlohmann::json>& tuples,
                                                    const std::string& user_prompt,
                                                    const ScalarFunctionType function_type, Model& model) {
    const auto llm_template = PromptManager::GetTemplate(function_type);

    int num_tokens_meta_and_user_prompt = 0;
    num_tokens_meta_and_user_prompt += Tiktoken::GetNumTokens(user_prompt);
    num_tokens_meta_and_user_prompt += Tiktoken::GetNumTokens(llm_template);
    const auto model_details = model.GetModelDetails();
    const auto available_tokens = model_details.context_window - num_tokens_meta_and_user_prompt;
    auto batch_size = model_details.batch_size;

    auto responses = nlohmann::json::array();

    if (available_tokens < 0) {
        throw std::runtime_error("The total number of tokens in the prompt exceeds the model's maximum token limit");
    } else {
        auto batch_tuples = nlohmann::json::array();
        int start_index = 0;

        if (batch_size == 0) {
            auto accumulated_tuples_tokens = 0u;
            batch_size = static_cast<int>(tuples.size());
            do {
                accumulated_tuples_tokens +=
                    Tiktoken::GetNumTokens(PromptManager::ConstructNumTuples(static_cast<int>(tuples.size())));
                accumulated_tuples_tokens +=
                    Tiktoken::GetNumTokens(PromptManager::ConstructInputTuplesHeader(tuples[start_index]));
                while (accumulated_tuples_tokens < static_cast<unsigned int>(available_tokens) &&
                       start_index < static_cast<int>(tuples.size()) && batch_tuples.size() < batch_size) {
                    const auto num_tokens =
                        Tiktoken::GetNumTokens(PromptManager::ConstructSingleInputTuple(tuples[start_index]));
                    if (accumulated_tuples_tokens + num_tokens > static_cast<unsigned int>(available_tokens)) {
                        break;
                    }
                    batch_tuples.push_back(tuples[start_index]);
                    accumulated_tuples_tokens += num_tokens;
                    start_index++;
                }

                nlohmann::json response;
                try {
                    response = Complete(batch_tuples, user_prompt, function_type, model);
                } catch (const ExceededMaxOutputTokensError&) {
                    batch_tuples.clear();
                    const auto new_batch_size = static_cast<int>(batch_size / 10);
                    batch_size = batch_size == 1 ? new_batch_size == 0 : new_batch_size;
                    accumulated_tuples_tokens = 0;
                    start_index = 0;
                    continue;
                }

                if (response.size() < batch_tuples.size()) {
                    for (auto i = static_cast<int>(response.size()); i < batch_tuples.size(); i++) {
                        response.push_back(nullptr);
                    }
                } else if (response.size() > batch_size) {
                    auto new_response = nlohmann::json::array();
                    for (auto i = 0; i < batch_size; i++) {
                        new_response.push_back(response[i]);
                    }
                    response = new_response;
                }

                auto output_tokens_per_tuple = Tiktoken::GetNumTokens(response.dump()) / batch_tuples.size();

                batch_size = model_details.max_output_tokens / static_cast<int>(output_tokens_per_tuple);
                batch_tuples.clear();
                accumulated_tuples_tokens = 0;

                for (const auto& tuple : response) {
                    responses.push_back(tuple);
                }

            } while (start_index < static_cast<int>(tuples.size()));
        } else {
            do {
                for (auto i = 0; i < batch_size; i++) {
                    if (start_index + i < static_cast<int>(tuples.size())) {
                        batch_tuples.push_back(tuples[start_index + i]);
                    }
                }
                start_index += batch_size;

                auto response = Complete(batch_tuples, user_prompt, function_type, model);

                if (response.size() < batch_tuples.size()) {
                    for (auto i = static_cast<int>(response.size()); i < batch_tuples.size(); i++) {
                        response.push_back(nullptr);
                    }
                } else if (response.size() > batch_size) {
                    auto new_response = nlohmann::json::array();
                    for (auto i = 0; i < batch_size; i++) {
                        new_response.push_back(response[i]);
                    }
                    response = new_response;
                }
                batch_tuples.clear();
                for (const auto& tuple : response) {
                    responses.push_back(tuple);
                }
            } while (start_index < static_cast<int>(tuples.size()));
        }
    }

    return responses;
}

} // namespace flockmtl
