#include "flock/model_manager/providers/adapters/anthropic.hpp"
#include <fmt/format.h>

namespace flock {

// Claude 4.x models support output_format, Claude 3.x models require tool_use fallback
// See: https://docs.anthropic.com/en/docs/build-with-claude/structured-outputs
static bool SupportsOutputFormat(const std::string& model) {
    // Claude 3.x models (claude-3-haiku, claude-3-sonnet, claude-3-opus, claude-3-5-sonnet, etc.)
    if (model.find("claude-3") != std::string::npos) {
        return false;
    }
    // Claude 4.x models (claude-sonnet-4-5, claude-haiku-4-5, claude-opus-4-5, etc.)
    return true;
}

void AnthropicProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) {

    auto message_content = nlohmann::json::array();
    message_content.push_back({{"type", "text"}, {"text", prompt}});

    if (!media_data.empty()) {
        for (const auto& column : media_data) {
            auto image_type = column.contains("type") ? column["type"].get<std::string>() : "image/png";
            auto media_type = std::string("image/");
            if (size_t pos = image_type.find("/"); pos != std::string::npos) {
                media_type = image_type;
            } else {
                media_type += std::string("png");
            }

            if (column.contains("data")) {
                for (const auto& image : column["data"]) {
                    auto image_str = image.get<std::string>();
                    if (!is_base64(image_str)) {
                        throw std::runtime_error("Anthropic requires base64-encoded images, not URLs.");
                    }
                    message_content.push_back({
                        {"type", "image"},
                        {"source", {{"type", "base64"}, {"media_type", media_type}, {"data", image_str}}}
                    });
                }
            }
        }
    }

    nlohmann::json request_payload = {{"model", model_details_.model},
                                      {"max_tokens", 1024},
                                      {"messages", {{{"role", "user"}, {"content", message_content}}}}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    // Build the schema for structured output
    nlohmann::json item_schema;
    if (model_details_.model_parameters.contains("output_format")) {
        item_schema = model_details_.model_parameters["output_format"]["schema"];
    } else {
        item_schema = {{"type", GetOutputTypeString(output_type)}};
    }

    if (SupportsOutputFormat(model_details_.model)) {
        // Claude 4.x: Use native output_format (preferred API)
        request_payload["output_format"] = {
                {"type", "json_schema"},
                {"schema", {{"type", "object"}, {"properties", {{"items", {{"type", "array"}, {"items", item_schema}}}}}, {"required", {"items"}}, {"additionalProperties", false}}}};
    } else {
        // Claude 3.x: Fall back to tool_use for structured output
        nlohmann::json flock_tool = {
            {"name", "flock_response"},
            {"description", "Return the structured response"},
            {"input_schema", {
                {"type", "object"},
                {"properties", {{"items", {{"type", "array"}, {"items", item_schema}}}}},
                {"required", nlohmann::json::array({"items"})}
            }}
        };
        request_payload["tools"] = nlohmann::json::array({flock_tool});
        request_payload["tool_choice"] = {{"type", "tool"}, {"name", "flock_response"}};
    }

    model_handler_->AddRequest(request_payload);
}

void AnthropicProvider::AddEmbeddingRequest(const std::vector<std::string>& inputs) {
    throw std::runtime_error("Anthropic does not support embeddings. Use OpenAI or Ollama.");
}

}// namespace flock
