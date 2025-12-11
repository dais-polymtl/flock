#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "session.hpp"
#include <cstdlib>
#include <curl/curl.h>

namespace flock {

class OllamaModelManager : public BaseModelProviderHandler {
public:
    OllamaModelManager(const std::string& url, const bool throw_exception)
        : BaseModelProviderHandler(throw_exception), _session("Ollama", throw_exception), _url(url) {}

    OllamaModelManager(const OllamaModelManager&) = delete;
    OllamaModelManager& operator=(const OllamaModelManager&) = delete;
    OllamaModelManager(OllamaModelManager&&) = delete;
    OllamaModelManager& operator=(OllamaModelManager&&) = delete;

protected:
    std::string getCompletionUrl() const override { return _url + "/api/chat"; }
    std::string getEmbedUrl() const override { return _url + "/api/embed"; }
    std::string getTranscriptionUrl() const override { return ""; }
    void prepareSessionForRequest(const std::string& url) override { _session.setUrl(url); }
    void setParameters(const std::string& data, const std::string& contentType = "") override {
        if (contentType != "multipart/form-data") {
            _session.setBody(data);
        }
    }
    auto postRequest(const std::string& contentType) -> decltype(((Session*) nullptr)->postPrepareOllama(contentType)) override {
        return _session.postPrepareOllama(contentType);
    }
    void checkProviderSpecificResponse(const nlohmann::json& response, RequestType request_type) override {
        if (request_type == RequestType::Transcription) {
            return;// No specific checks needed for transcriptions
        }
        bool is_completion = (request_type == RequestType::Completion);
        if (is_completion) {
            if (response.contains("done_reason") && response["done_reason"] != "stop") {
                throw std::runtime_error("The request was refused due to some internal error with Ollama API");
            }
            if (response.contains("done") && !response["done"].is_null() && !response["done"].get<bool>()) {
                throw std::runtime_error("The request was not completed by Ollama API");
            }
        } else {
            if (response.contains("embeddings") && (!response["embeddings"].is_array() || response["embeddings"].empty())) {
                throw std::runtime_error("Ollama API returned empty or invalid embedding data.");
            }
        }
    }

    nlohmann::json ExtractCompletionOutput(const nlohmann::json& response) const override {
        if (response.contains("message") && response["message"].is_object()) {
            const auto& message = response["message"];
            if (message.contains("content")) {
                const auto& content = message["content"];
                if (content.is_null()) {
                    std::cerr << "Error: Ollama API returned null content in message. Full response: " << response.dump(2) << std::endl;
                    throw std::runtime_error("Ollama API returned null content in message. Response: " + response.dump());
                }
                if (content.is_string()) {
                    try {
                        auto parsed = nlohmann::json::parse(content.get<std::string>());
                        // Validate that parsed result has expected structure for aggregate functions
                        if (!parsed.contains("items") || !parsed["items"].is_array()) {
                            std::cerr << "Warning: Parsed content does not contain 'items' array. Parsed: " << parsed.dump(2) << std::endl;
                        }
                        return parsed;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: Failed to parse Ollama response content as JSON: " << e.what() << std::endl;
                        std::cerr << "Content was: " << content.dump() << std::endl;
                        throw std::runtime_error("Failed to parse Ollama response as JSON: " + std::string(e.what()) + ". Content: " + content.dump());
                    }
                } else {
                    // Content might already be a JSON object
                    // Validate structure
                    if (!content.contains("items") || !content["items"].is_array()) {
                        std::cerr << "Warning: Content does not contain 'items' array. Content: " << content.dump(2) << std::endl;
                    }
                    return content;
                }
            } else {
                std::cerr << "Error: Ollama API response missing 'content' field in message. Full response: " << response.dump(2) << std::endl;
                throw std::runtime_error("Ollama API response missing message.content field. Response: " + response.dump());
            }
        } else {
            std::cerr << "Error: Ollama API response missing 'message' object. Full response: " << response.dump(2) << std::endl;
            throw std::runtime_error("Ollama API response missing message field. Response: " + response.dump());
        }
    }

    nlohmann::json ExtractEmbeddingVector(const nlohmann::json& response) const override {
        if (response.contains("embeddings") && response["embeddings"].is_array()) {
            return response["embeddings"];
        }
        return {};
    }

    std::pair<int64_t, int64_t> ExtractTokenUsage(const nlohmann::json& response) const override {
        int64_t input_tokens = 0;
        int64_t output_tokens = 0;
        if (response.contains("prompt_eval_count") && response["prompt_eval_count"].is_number()) {
            input_tokens = response["prompt_eval_count"].get<int64_t>();
        }
        if (response.contains("eval_count") && response["eval_count"].is_number()) {
            output_tokens = response["eval_count"].get<int64_t>();
        }
        return {input_tokens, output_tokens};
    }


    nlohmann::json ExtractTranscriptionOutput(const nlohmann::json& response) const override {
        throw std::runtime_error("Audio transcription is not supported for Ollama provider, use Azure or OpenAI instead.");
    }


    Session _session;
    std::string _url;
};

}// namespace flock
