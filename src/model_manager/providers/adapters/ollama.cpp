#include "flockmtl/model_manager/providers/adapters/ollama.hpp"

namespace flockmtl {

nlohmann::json OllamaProvider::CallComplete(const std::string& prompt, const bool json_response, OutputType output_type) {
    // Create a JSON request payload with the provided parameters
    nlohmann::json request_payload = {{"model", model_details_.model},
                                      {"prompt", prompt}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    if (json_response) {
        if (model_details_.model_parameters.contains("format")) {
            auto schema = model_details_.model_parameters["format"];
            request_payload["format"] = {{"type", "object"}, {"properties", {"items", {{"type", "array"}, {"items", schema}}}}};
        } else {
            request_payload["format"] = {{"type", "object"}, {"properties", {"items", {{"type", "array"}, {"items", {{"type", GetOutputTypeString(output_type)}}}}}}};
        }
    }

    nlohmann::json completion;
    try {
        completion = model_handler_->CallComplete(request_payload);
    } catch (const std::exception& e) {
        throw std::runtime_error(duckdb_fmt::format("Error in making request to Ollama API: {}", e.what()));
    }

    // Check if the call was not succesfull
    if ((completion.contains("done_reason") && completion["done_reason"] != "stop") ||
        (completion.contains("done") && !completion["done"].is_null() && completion["done"].get<bool>() != true)) {
        // Handle refusal error
        throw std::runtime_error("The request was refused due to some internal error with Ollama API");
    }

    std::string content_str = completion["response"];

    if (json_response) {
        return nlohmann::json::parse(content_str);
    }

    return content_str;
}

nlohmann::json OllamaProvider::CallEmbedding(const std::vector<std::string>& inputs) {
    auto embeddings = nlohmann::json::array();
    for (const auto& input: inputs) {
        // Create a JSON request payload with the provided parameters
        nlohmann::json request_payload = {
                {"model", model_details_.model},
                {"prompt", input},
        };

        nlohmann::json completion;
        try {
            completion = model_handler_->CallEmbedding(request_payload);
        } catch (const std::exception& e) {
            throw std::runtime_error(duckdb_fmt::format("Error in making request to Ollama API: {}", e.what()));
        }

        embeddings.push_back(completion["embedding"]);
    }
    return embeddings;
}

}// namespace flockmtl