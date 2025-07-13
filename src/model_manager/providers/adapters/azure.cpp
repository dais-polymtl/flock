#include "flockmtl/model_manager/providers/adapters/azure.hpp"

namespace flockmtl {

nlohmann::json AzureProvider::CallComplete(const std::string& prompt, const bool json_response, OutputType output_type) {
    // Create a JSON request payload with the provided parameters
    nlohmann::json request_payload = {{"messages", {{{"role", "user"}, {"content", prompt}}}}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    if (json_response) {
        if (model_details_.model_parameters.contains("response_format")) {
            auto schema = model_details_.model_parameters["response_format"]["json_schema"]["schema"];
            auto strict = model_details_.model_parameters["response_format"]["strict"];
            request_payload["response_format"] = {
                    {"type", "json_schema"},
                    {"json_schema",
                     {{"name", "flockmtl_response"},
                      {"strict", strict},
                      {"schema", {{"type", "object"}, {"properties", {{"items", {{"type", "array"}, {"items", schema}}}}}, {"required", {"items"}}, {"additionalProperties", false}}}}}};
        } else {
            request_payload["response_format"] = {
                    {"type", "json_schema"},
                    {"json_schema",
                     {{"name", "flockmtl_response"},
                      {"strict", false},
                      {"schema", {{"type", "object"}, {"properties", {{"items", {{"type", "array"}, {"items", {{"type", GetOutputTypeString(output_type)}}}}}}}}}}}};
            ;
        }
    }

    // Make a request to the Azure API
    auto completion = model_handler_->CallComplete(request_payload);

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
    // Create a JSON request payload with the provided parameters
    nlohmann::json request_payload = {
            {"model", model_details_.model},
            {"input", inputs},
    };

    // Make a request to the Azure API
    auto completion = model_handler_->CallEmbedding(request_payload);

    // Check if the conversation was too long for the context window
    if (completion["choices"][0]["finish_reason"] == "length") {
        // Handle the error when the context window is too long
        throw ExceededMaxOutputTokensError();
        // Add error handling code here
    }

    auto embeddings = nlohmann::json::array();
    for (auto& item: completion["data"]) {
        embeddings.push_back(item["embedding"]);
    }

    return embeddings;
}

}// namespace flockmtl