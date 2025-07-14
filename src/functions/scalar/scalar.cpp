#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

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

nlohmann::json ScalarFunctionBase::BatchAndComplete(const std::vector<nlohmann::json>& tuples,
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
