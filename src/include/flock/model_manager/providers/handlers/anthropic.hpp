#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "flock/model_manager/providers/provider.hpp"
#include "session.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace flock {

class AnthropicModelManager : public BaseModelProviderHandler {
public:
    AnthropicModelManager(std::string api_key, std::string api_version, bool throw_exception,
                          const std::string& model_name = "",
                          std::optional<int> rate_limit = std::nullopt,
                          std::optional<UsageLimit> usage_limit = std::nullopt,
                          std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                          std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr,
                          std::string custom_api_url = "")
        : BaseModelProviderHandler(throw_exception, model_name, rate_limit, std::move(usage_limit),
                                   std::move(rate_limiter), std::move(usage_limiter)),
          _api_key(std::move(api_key)),
          _api_version(std::move(api_version)),
          _session("Anthropic", throw_exception),
          _custom_api_url(std::move(custom_api_url)) {
        if (!_custom_api_url.empty()) {
            _api_base_url = _custom_api_url;
            // Ensure trailing slash for URL concatenation with getCompletionUrl
            if (_api_base_url.back() != '/') {
                _api_base_url += '/';
            }
        } else {
            _api_base_url = "https://api.anthropic.com/v1/";
        }
        _session.setUrl(_api_base_url);
    }

    AnthropicModelManager(const AnthropicModelManager&) = delete;
    AnthropicModelManager& operator=(const AnthropicModelManager&) = delete;
    AnthropicModelManager(AnthropicModelManager&&) = delete;
    AnthropicModelManager& operator=(AnthropicModelManager&&) = delete;

protected:
    std::string _api_key;
    std::string _api_version;
    std::string _api_base_url;
    std::string _custom_api_url;
    Session _session;

    std::string getCompletionUrl() const override {
        return _api_base_url + "messages";
    }

    std::string getEmbedUrl() const override {
        throw std::runtime_error("Anthropic does not support embeddings.");
    }

    std::string getTranscriptionUrl() const override {
        throw std::runtime_error("Anthropic does not support audio transcription.");
    }

    void prepareSessionForRequest(const std::string& url) override {
        _session.setUrl(url);
    }

    void setParameters(const std::string& data, const std::string& contentType = "") override {
        if (contentType != "multipart/form-data") {
            _session.setBody(data);
        }
    }

    auto postRequest(const std::string& contentType) -> decltype(((Session*) nullptr)->postPrepare(contentType)) override {
        return _session.postPrepare(contentType);
    }

    std::vector<std::string> getExtraHeaders() const override {
        return {
                "x-api-key: " + _api_key,
                "anthropic-version: " + _api_version,
                "anthropic-beta: structured-outputs-2025-11-13"};
    }

    void checkProviderSpecificResponse(const nlohmann::json& response, RequestType request_type) override {
        if (request_type != RequestType::Completion) {
            throw std::runtime_error("Anthropic does not support embeddings or transcriptions.");
        }
        if (response.contains("type") && response["type"] == "error") {
            std::string error_msg = "Anthropic API error";
            if (response.contains("error") && response["error"].contains("message")) {
                error_msg = response["error"]["message"].get<std::string>();
            }
            ThrowIfTokenLimitProviderError(error_msg);
            throw std::runtime_error("Anthropic API error: " + error_msg);
        }
        if (response.contains("stop_reason") && !response["stop_reason"].is_null()) {
            std::string stop_reason = response["stop_reason"].get<std::string>();
            if (stop_reason == "max_tokens") {
                throw TokenLimitExceededError();
            }
            if (stop_reason != "end_turn" && stop_reason != "stop_sequence" && stop_reason != "tool_use") {
                throw std::runtime_error("Anthropic API unexpected stop_reason: " + stop_reason);
            }
        }
    }

    nlohmann::json ExtractCompletionOutput(const nlohmann::json& response) const override {

        if (!response.contains("content") || !response["content"].is_array() || response["content"].empty()) {

            return {};
        }
        const auto& content = response["content"];

        // First, check for tool_use blocks (Claude 3.x fallback)
        for (const auto& block: content) {
            if (block.contains("type") && block["type"] == "tool_use" && block.contains("input")) {
                auto input = block["input"];
                if (input.contains("items") && !input["items"].is_array()) {
                    input["items"] = nlohmann::json::array({input["items"]});
                }
                return input;
            }
        }

        // Then, check for text blocks (Claude 4.x with output_format)
        for (const auto& block: content) {
            if (block.contains("type") && block["type"] == "text" && block.contains("text")) {
                std::string text = block["text"].get<std::string>();
                try {
                    auto parsed = nlohmann::json::parse(text);
                    if (parsed.contains("items") && !parsed["items"].is_array()) {
                        parsed["items"] = nlohmann::json::array({parsed["items"]});
                    }
                    return parsed;
                } catch (...) {
                    return nlohmann::json({{"items", nlohmann::json::array({text})}});
                }
            }
        }

        // Finally, fall back to thinking blocks (Qwen models via Ollama's Anthropic API)
        for (const auto& block: content) {
            if (block.contains("type") && block["type"] == "thinking" && block.contains("thinking")) {
                std::string text = block["thinking"].get<std::string>();
                try {
                    auto parsed = nlohmann::json::parse(text);
                    if (parsed.contains("items") && !parsed["items"].is_array()) {
                        parsed["items"] = nlohmann::json::array({parsed["items"]});
                    }
                    return parsed;
                } catch (...) {
                    return nlohmann::json({{"items", nlohmann::json::array({text})}});
                }
            }
        }

        return {};
    }

    nlohmann::json ExtractEmbeddingVector(const nlohmann::json& response) const override {
        throw std::runtime_error("Anthropic does not support embeddings.");
    }

    nlohmann::json ExtractTranscriptionOutput(const nlohmann::json& response) const override {
        (void) response;
        throw std::runtime_error("Anthropic does not support audio transcription.");
    }

    std::pair<int64_t, int64_t> ExtractTokenUsage(const nlohmann::json& response) const override {
        int64_t input_tokens = 0;
        int64_t output_tokens = 0;
        if (response.contains("usage") && response["usage"].is_object()) {
            const auto& usage = response["usage"];
            if (usage.contains("input_tokens") && usage["input_tokens"].is_number()) {
                input_tokens = usage["input_tokens"].get<int64_t>();
            }
            if (usage.contains("output_tokens") && usage["output_tokens"].is_number()) {
                output_tokens = usage["output_tokens"].get<int64_t>();
            }
        }
        return {input_tokens, output_tokens};
    }

    // Anthropic streaming format uses event types:
    // message_start: {"type":"message_start","message":{"...","usage":{"input_tokens":N}}, ...}
    // content_block_start: {"type":"content_block_start","index":0,...}
    // content_block_delta: {"type":"content_block_delta","delta":{"type":"text_delta","text":"..."}}
    //                    or: {"type":"content_block_delta","delta":{"type":"input_json_delta","partial_json":"..."}}
    //                    (for tool_use, input_json_delta carries the partial JSON arguments)
    // message_delta: {"type":"message_delta","delta":{"stop_reason":"..."},"usage":{"output_tokens":N}}
    // message_stop: {"type":"message_stop"}
    nlohmann::json ReconstructFromStreamedChunks(const std::string& sse_raw) const override {
        std::string accumulated_content;
        std::string accumulated_input_json;
        std::string finish_reason;
        int64_t input_tokens = 0;
        int64_t output_tokens = 0;
        bool has_tool_use = false;

        std::istringstream stream(sse_raw);
        std::string line;
        std::string current_event;

        while (std::getline(stream, line)) {
            // Trim
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();

            if (line.rfind("event: ", 0) == 0) {
                current_event = line.substr(7);
                continue;
            }
            if (line.rfind("data: ", 0) == 0) {
                std::string json_str = line.substr(6);
                if (json_str.empty() || json_str[0] != '{') continue;

                nlohmann::json chunk;
                try {
                    chunk = nlohmann::json::parse(json_str);
                } catch (...) { continue; }

                if (chunk.contains("error")) return nlohmann::json();

                // message_start: get input_tokens
                if (current_event == "message_start" && chunk.contains("message")) {
                    const auto& msg = chunk["message"];
                    if (msg.contains("usage") && msg["usage"].is_object()) {
                        const auto& usage = msg["usage"];
                        if (usage.contains("input_tokens") && usage["input_tokens"].is_number()) {
                            input_tokens = usage["input_tokens"].get<int64_t>();
                        }
                    }
                }

                // content_block_delta: accumulate text or tool input JSON
                if (current_event == "content_block_delta" && chunk.contains("delta")) {
                    auto& delta = chunk["delta"];
                    // Text content from text_delta
                    if (delta.contains("text") && delta["text"].is_string()) {
                        accumulated_content += delta["text"].get<std::string>();
                    }
                    // Thinking content from thinking_delta (skip - it's reasoning, not the answer)
                    if (delta.contains("type") && delta["type"] == "thinking_delta") {
                        // Intentionally skip thinking content
                    }
                    // Tool call arguments from input_json_delta (streaming tool_use)
                    if (delta.contains("type") && delta["type"] == "input_json_delta" && delta.contains("partial_json")) {
                        has_tool_use = true;
                        accumulated_input_json += delta["partial_json"].get<std::string>();
                    }
                }

                // message_delta: get stop_reason and output_tokens
                if (current_event == "message_delta") {
                    if (chunk.contains("delta") && chunk["delta"].contains("stop_reason")) {
                        auto& sr = chunk["delta"]["stop_reason"];
                        if (sr.is_string()) finish_reason = sr.get<std::string>();
                    }
                    if (chunk.contains("usage") && chunk["usage"].is_object()) {
                        const auto& usage = chunk["usage"];
                        if (usage.contains("output_tokens") && usage["output_tokens"].is_number()) {
                            output_tokens = usage["output_tokens"].get<int64_t>();
                        }
                    }
                }
            }
        }

        if (accumulated_content.empty() && accumulated_input_json.empty()) {
            return nlohmann::json();
        }

        nlohmann::json reconstructed;

        if (has_tool_use && !accumulated_input_json.empty()) {
            // Reconstruct tool_use response for streaming structured output
            // Non-streaming format: {"content": [{"type": "tool_use", "id": "...", "name": "flock_response", "input": {...}}]}
            try {
                auto input_json = nlohmann::json::parse(accumulated_input_json);
                if (input_json.contains("items") && !input_json["items"].is_array()) {
                    input_json["items"] = nlohmann::json::array({input_json["items"]});
                }
                nlohmann::json tool_block;
                tool_block["type"] = "tool_use";
                tool_block["id"] = "call_0";
                tool_block["name"] = "flock_response";
                tool_block["input"] = input_json;
                reconstructed["content"] = nlohmann::json::array({tool_block});
            } catch (...) {
                // If input JSON parsing fails, fall back to text
                nlohmann::json text_block;
                text_block["type"] = "text";
                text_block["text"] = accumulated_input_json;
                reconstructed["content"] = nlohmann::json::array({text_block});
            }
        } else {
            // Reconstruct in the same shape Anthropic's non-streaming response would have
            // (content array with text blocks that contain JSON strings)
            nlohmann::json text_block;
            text_block["type"] = "text";
            text_block["text"] = accumulated_content;
            reconstructed["content"] = nlohmann::json::array({text_block});
        }

        if (!finish_reason.empty()) reconstructed["stop_reason"] = finish_reason;

        if (input_tokens > 0 || output_tokens > 0) {
            reconstructed["usage"] = {
                    {"input_tokens", input_tokens},
                    {"output_tokens", output_tokens}};
        }

        return reconstructed;
    }
};

}// namespace flock
