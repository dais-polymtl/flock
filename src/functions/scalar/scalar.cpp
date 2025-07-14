#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

nlohmann::json ScalarFunctionBase::Complete(const nlohmann::json& tuples, const std::string& user_prompt,
                                            ScalarFunctionType function_type, Model& model) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_prompt, tuples, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    if (function_type == ScalarFunctionType::FILTER) {
        output_type = OutputType::BOOL;
    }
    model.AddCompletionRequest(prompt, true, output_type);
    auto response = model.CollectCompletions();
    return response[0]["items"];
};

template<>
nlohmann::json ScalarFunctionBase::BatchAndComplete<ExecutionMode::SYNC>(const std::vector<nlohmann::json>& tuples,
                                                                         const std::string& user_prompt,
                                                                         const ScalarFunctionType function_type, Model& model) {
    const auto llm_template = PromptManager::GetTemplate(function_type);

    const auto model_details = model.GetModelDetails();
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples.size()));

    auto responses = nlohmann::json::array();

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;

    do {
        batch_tuples.clear();

        for (auto i = 0; i < batch_size && start_index + i < static_cast<int>(tuples.size()); i++) {
            batch_tuples.push_back(tuples[start_index + i]);
        }

        start_index += batch_size;

        try {
            auto response = Complete(batch_tuples, user_prompt, function_type, model);

            if (response.size() < batch_tuples.size()) {
                for (auto i = static_cast<int>(response.size()); i < batch_tuples.size(); i++) {
                    response.push_back(nullptr);
                }
            }

            for (const auto& tuple: response) {
                responses.push_back(tuple);
            }
        } catch (const ExceededMaxOutputTokensError&) {
            start_index -= batch_size;
            batch_size = static_cast<int>(batch_size * 0.9);
            if (batch_size <= 0) {
                throw std::runtime_error("Batch size reduced to zero, unable to process tuples");
            }
        }

    } while (start_index < static_cast<int>(tuples.size()));

    return responses;
}

void ScalarFunctionBase::AddCompletionRequest(const nlohmann::json& tuples, const std::string& user_prompt,
                                              ScalarFunctionType function_type, Model& model) {
    nlohmann::json data;
    const auto prompt = PromptManager::Render(user_prompt, tuples, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    if (function_type == ScalarFunctionType::FILTER) {
        output_type = OutputType::BOOL;
    }
    model.AddCompletionRequest(prompt, true, output_type);
};

template<>
nlohmann::json ScalarFunctionBase::BatchAndComplete<ExecutionMode::ASYNC>(const std::vector<nlohmann::json>& tuples,
                                                                          const std::string& user_prompt,
                                                                          const ScalarFunctionType function_type, Model& model) {
    const auto llm_template = PromptManager::GetTemplate(function_type);
    const auto model_details = model.GetModelDetails();
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples.size()));
    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    std::vector<std::vector<nlohmann::json>> all_batches;
    int start_index = 0;
    // Create all batches first
    while (start_index < static_cast<int>(tuples.size())) {
        std::vector<nlohmann::json> batch;
        for (auto i = 0; i < batch_size && start_index + i < static_cast<int>(tuples.size()); i++) {
            batch.push_back(tuples[start_index + i]);
        }
        all_batches.push_back(batch);
        start_index += batch_size;
    }

    // For each batch, call Complete (which should AddRequest internally)
    for (const auto& batch: all_batches) {
        AddCompletionRequest(batch, user_prompt, function_type, model);
    }

    // After all requests are queued, send them all and collect results
    auto responses = model.CollectCompletions();
    auto results = nlohmann::json::array();
    for (auto& response: responses) {
        auto items = response["items"];
        for (const auto& item: items) {
            results.push_back(item);
        }
    }
    return results;
}

}// namespace flockmtl
