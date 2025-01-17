#include "flockmtl/model_manager/providers/adapters/azure.hpp"

namespace flockmtl {

nlohmann::json AzureProvider::CallComplete(const std::string& prompt, const bool json_response) {
    auto azure_model_manager_uptr =
        std::make_unique<AzureModelManager>(model_details_.secret["api_key"], model_details_.secret["resource_name"],
                                            model_details_.model, model_details_.secret["api_version"], true);

    // Create a JSON request payload with the provided parameters
    nlohmann::json request_payload = {{"messages", {{{"role", "user"}, {"content", prompt}}}},
                                      {"max_tokens", model_details_.max_output_tokens},
                                      {"temperature", model_details_.temperature}};

    // Conditionally add "response_format" if json_response is true
    if (json_response) {
        request_payload["response_format"] = {{"type", "json_object"}};
    }

    // Make a request to the Azure API
    auto completion = azure_model_manager_uptr->CallComplete(request_payload);

    // Check if the conversation was too long for the context window
    if (completion["choices"][0]["finish_reason"] == "length") {
        // Handle the error when the context window is too long
        throw ExceededMaxOutputTokensError();
    }

    // Check if the safety system refused the request
    if (completion["choices"][0]["message"]["refusal"] != nullptr) {
        // Handle refusal error
        throw std::runtime_error(
            duckdb_fmt::format("The request was refused due to Azure's safety system.{{\"refusal\": \"{}\"}}",
                               completion["choices"][0]["message"]["refusal"].get<std::string>()));
    }

    // Check if the model's output included restricted content
    if (completion["choices"][0]["finish_reason"] == "content_filter") {
        // Handle content filtering
        throw std::runtime_error("The content filter was triggered, resulting in incomplete JSON.");
    }

    std::string content_str = completion["choices"][0]["message"]["content"];

    if (json_response) {
        return nlohmann::json::parse(content_str);
    }

    return content_str;
}

nlohmann::json AzureProvider::CallEmbedding(const std::vector<std::string>& inputs) {
    auto azure_model_manager_uptr =
        std::make_unique<AzureModelManager>(model_details_.secret["api_key"], model_details_.secret["resource_name"],
                                            model_details_.model, model_details_.secret["api_version"], true);

    // Create a JSON request payload with the provided parameters
    nlohmann::json request_payload = {
        {"model", model_details_.model},
        {"input", inputs},
    };

    // Make a request to the Azure API
    auto completion = azure_model_manager_uptr->CallEmbedding(request_payload);

    // Check if the conversation was too long for the context window
    if (completion["choices"][0]["finish_reason"] == "length") {
        // Handle the error when the context window is too long
        throw ExceededMaxOutputTokensError();
        // Add error handling code here
    }

    auto embeddings = nlohmann::json::array();
    for (auto& item : completion["data"]) {
        embeddings.push_back(item["embedding"]);
    }

    return embeddings;
}

} // namespace flockmtl