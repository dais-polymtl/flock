#include "flock/functions/scalar/scalar.hpp"

namespace flock {

nlohmann::json ScalarFunctionBase::Complete(nlohmann::json& columns, const std::string& user_prompt,
                                            ScalarFunctionType function_type, Model& model) {
    const auto [prompt, media_data] = PromptManager::Render(user_prompt, columns, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    if (function_type == ScalarFunctionType::FILTER) {
        output_type = OutputType::BOOL;
    }

    model.AddCompletionRequest(prompt, static_cast<int>(columns[0]["data"].size()), output_type, media_data);
    auto response = model.CollectCompletions();
    return response[0]["items"];
};

nlohmann::json ScalarFunctionBase::BatchAndComplete(const nlohmann::json& tuples,
                                                    const std::string& user_prompt,
                                                    const ScalarFunctionType function_type, Model& model) {
    const auto llm_template = PromptManager::GetTemplate(function_type);

    const auto model_details = model.GetModelDetails();
    auto batch_size = std::min<int>(model.GetModelDetails().batch_size, static_cast<int>(tuples[0]["data"].size()));

    auto responses = nlohmann::json::array();

    if (batch_size <= 0) {
        throw std::runtime_error("Batch size must be greater than zero");
    }

    auto batch_tuples = nlohmann::json::array();
    int start_index = 0;

    do {
        batch_tuples.clear();

        for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
            batch_tuples.push_back(nlohmann::json::object());
            for (const auto& item: tuples[i].items()) {
                if (item.key() != "data") {
                    batch_tuples[i][item.key()] = item.value();
                } else {
                    for (auto j = 0; j < batch_size && start_index + j < static_cast<int>(item.value().size()); j++) {
                        if (j == 0) {
                            batch_tuples[i]["data"] = nlohmann::json::array();
                        }
                        batch_tuples[i]["data"].push_back(item.value()[start_index + j]);
                    }
                }
            }
        }

        start_index += batch_size;

        try {
            auto response = Complete(batch_tuples, user_prompt, function_type, model);

            if (response.size() < batch_tuples[0]["data"].size()) {
                for (auto i = static_cast<int>(response.size()); i < batch_tuples[0]["data"].size(); i++) {
                    response.push_back(nullptr);
                }
            } else if (response.size() > batch_tuples[0]["data"].size()) {
                response.erase(response.begin() + batch_tuples.size(), response.end());
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

    } while (start_index < static_cast<int>(tuples[0]["data"].size()));

    return responses;
}

}// namespace flock
