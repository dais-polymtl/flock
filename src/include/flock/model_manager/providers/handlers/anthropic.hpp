#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "flock/model_manager/providers/provider.hpp"
#include "session.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace flock {

class AnthropicModelManager : public BaseModelProviderHandler {
public:
    AnthropicModelManager(std::string api_key, std::string api_version, bool throw_exception)
        : BaseModelProviderHandler(throw_exception),
          _api_key(std::move(api_key)),
          _api_version(std::move(api_version)),
          _session("Anthropic", throw_exception) {
        _api_base_url = "https://api.anthropic.com/v1/";
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
    Session _session;

    std::string getCompletionUrl() const override {
        return _api_base_url + "messages";
    }

    std::string getEmbedUrl() const override {
        throw std::runtime_error("Anthropic does not support embeddings.");
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
            "anthropic-beta: structured-outputs-2025-11-13"
        };
    }

    void checkProviderSpecificResponse(const nlohmann::json& response, bool is_completion) override {
        if (!is_completion) {
            throw std::runtime_error("Anthropic does not support embeddings.");
        }
        if (response.contains("type") && response["type"] == "error") {
            std::string error_msg = "Anthropic API error";
            if (response.contains("error") && response["error"].contains("message")) {
                error_msg = response["error"]["message"].get<std::string>();
            }
            throw std::runtime_error("Anthropic API error: " + error_msg);
        }
        if (response.contains("stop_reason") && !response["stop_reason"].is_null()) {
            std::string stop_reason = response["stop_reason"].get<std::string>();
            if (stop_reason == "max_tokens") {
                throw ExceededMaxOutputTokensError();
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
        for (const auto& block : content) {
            if (block.contains("type") && block["type"] == "tool_use" && block.contains("input")) {
                auto input = block["input"];
                if (input.contains("items") && !input["items"].is_array()) {
                    input["items"] = nlohmann::json::array({input["items"]});
                }
                return input;
            }
        }

        // Then, check for text blocks (Claude 4.x with output_format)
        for (const auto& block : content) {
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
        return {};
    }

    nlohmann::json ExtractEmbeddingVector(const nlohmann::json& response) const override {
        throw std::runtime_error("Anthropic does not support embeddings.");
    }
};

}// namespace flock
