#include "flock/model_manager/providers/adapters/ollama.hpp"
#include "flock/model_manager/providers/handlers/url_handler.hpp"
#include "flock/model_manager/providers/provider.hpp"

namespace flock {

void OllamaProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) {
    // Build message for chat API
    nlohmann::json message = {{"role", "user"}, {"content", prompt}};

    // Process image columns - images go in the message object as an "images" array
    auto images = nlohmann::json::array();
    if (media_data.contains("image") && !media_data["image"].empty() && media_data["image"].is_array()) {
        for (const auto& column: media_data["image"]) {
            if (column.contains("data") && column["data"].is_array()) {
                for (const auto& image: column["data"]) {
                    // Skip null values
                    if (image.is_null()) {
                        continue;
                    }
                    std::string image_str;
                    if (image.is_string()) {
                        image_str = image.get<std::string>();
                    } else {
                        // Convert non-string values to string
                        image_str = image.dump();
                    }

                    // Handle file path or URL - resolve and convert to base64
                    auto base64_result = URLHandler::ResolveFileToBase64(image_str);
                    images.push_back(base64_result.base64_content);
                }
            }
        }
    }
    if (!images.empty()) {
        message["images"] = images;
    }

    nlohmann::json request_payload = {{"model", model_details_.model},
                                      {"messages", nlohmann::json::array({message})},
                                      {"stream", false}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    if (model_details_.model_parameters.contains("format")) {
        auto schema = model_details_.model_parameters["format"];
        request_payload["format"] = {
                {"type", "object"},
                {"properties", {{"items", {{"type", "array"}, {"minItems", num_output_tuples}, {"maxItems", num_output_tuples}, {"items", schema}}}}},
                {"required", {"items"}}};
    } else {
        request_payload["format"] = {
                {"type", "object"},
                {"properties", {{"items", {{"type", "array"}, {"minItems", num_output_tuples}, {"maxItems", num_output_tuples}, {"items", {{"type", GetOutputTypeString(output_type)}}}}}}},
                {"required", {"items"}}};
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

void OllamaProvider::AddTranscriptionRequest(const nlohmann::json& audio_files) {
    throw std::runtime_error("Audio transcription is not currently supported by Ollama.");
}

}// namespace flock