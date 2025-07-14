#include "flockmtl/model_manager/providers/adapters/ollama.hpp"

namespace flockmtl {

void OllamaProvider::AddCompletionRequest(const std::string& prompt, const bool json_response, OutputType output_type) {
    nlohmann::json request_payload = {{"model", model_details_.model},
                                      {"prompt", prompt},
                                      {"stream", false}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    if (json_response) {
        if (model_details_.model_parameters.contains("format")) {
            auto schema = model_details_.model_parameters["format"];
            request_payload["format"] = {
                    {"type", "object"},
                    {"properties", {{"items", {{"type", "array"}, {"items", schema}}}}},
                    {"required", {"items"}}};
        } else {
            request_payload["format"] = {
                    {"type", "object"},
                    {"properties", {{"items", {{"type", "array"}, {"items", {{"type", GetOutputTypeString(output_type)}}}}}}},
                    {"required", {"items"}}};
        }
    }

    model_handler_->AddRequest(request_payload);
}

void OllamaProvider::AddEmbeddingRequest(const std::vector<std::string>& inputs) {
    for (const auto& input: inputs) {
        nlohmann::json request_payload = {
                {"model", model_details_.model},
                {"input", input},
        };

        model_handler_->AddRequest(request_payload, IModelProviderHandler::RequestType::Embedding);
    }
}

}// namespace flockmtl