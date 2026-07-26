#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "session.hpp"
#include <cstdlib>
#include <curl/curl.h>
#include <sstream>

namespace flock {

class OllamaModelManager : public BaseModelProviderHandler {
public:
    OllamaModelManager(const std::string& url, const bool throw_exception, const std::string& model_name = "",
                       std::optional<int> rate_limit = std::nullopt,
                       std::optional<UsageLimit> usage_limit = std::nullopt,
                       std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                       std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : BaseModelProviderHandler(throw_exception, model_name, rate_limit, std::move(usage_limit),
                                   std::move(rate_limiter), std::move(usage_limiter)),
          _session("Ollama", throw_exception), _url(url) {}

    OllamaModelManager(const OllamaModelManager&) = delete;
    OllamaModelManager& operator=(const OllamaModelManager&) = delete;
    OllamaModelManager(OllamaModelManager&&) = delete;
    OllamaModelManager& operator=(OllamaModelManager&&) = delete;

protected:
    std::string getCompletionUrl() const override { return _url + "/api/chat"; }
    std::string getEmbedUrl() const override { return _url + "/api/embed"; }
    std::string getTranscriptionUrl() const override { return ""; }
    std::vector<std::string> getExtraHeaders() const override {
        // Add Connection: close to prevent curl_multi_wait from hanging on keepalive
        return {"Connection: close"};
    }
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

    // Ollama streaming format (native /api/chat): plain JSON lines, not SSE.
    // Each line is a complete JSON object: {"message":{"content":"..."},"done":false}
    nlohmann::json ReconstructFromStreamedChunks(const std::string& sse_raw) const override {
        std::string accumulated_content;
        std::string finish_reason;
        int64_t input_tokens = 0;
        int64_t output_tokens = 0;
        bool done = false;

        std::istringstream stream(sse_raw);
        std::string line;

        while (std::getline(stream, line)) {
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();

            if (line.empty() || line[0] != '{') continue;

            nlohmann::json chunk;
            try {
                chunk = nlohmann::json::parse(line);
            } catch (...) { continue; }

            if (chunk.contains("message") && chunk["message"].is_object()) {
                // When think=true, the model outputs to BOTH fields:
                // - thinking: raw trace (reasoning + answer)
                // - content: clean answer (only at end)
                // Since content always exists (even empty), first if matches during thinking,
                // and we only accumulate the clean content at the end.
                if (chunk["message"].contains("content") && chunk["message"]["content"].is_string()) {
                    accumulated_content += chunk["message"]["content"].get<std::string>();
                }
            }

            if (chunk.contains("done")) done = chunk["done"].get<bool>();

            // Capture finish_reason and usage from the last chunk
            if (chunk.contains("error")) {
                return nlohmann::json();
            }
            if (done) {
                if (chunk.contains("done_reason")) {
                    auto& dr = chunk["done_reason"];
                    if (dr.is_string()) finish_reason = dr.get<std::string>();
                }
                if (chunk.contains("prompt_eval_count")) input_tokens = chunk["prompt_eval_count"].get<int64_t>();
                if (chunk.contains("eval_count")) output_tokens = chunk["eval_count"].get<int64_t>();
            }
        }

        if (accumulated_content.empty()) {
            return nlohmann::json();
        }

        nlohmann::json reconstructed;
        reconstructed["message"]["content"] = accumulated_content;
        reconstructed["message"]["role"] = "assistant";
        if (!finish_reason.empty()) {
            reconstructed["done_reason"] = finish_reason;
        }
        reconstructed["done"] = true;
        if (input_tokens > 0) reconstructed["prompt_eval_count"] = input_tokens;
        if (output_tokens > 0) reconstructed["eval_count"] = output_tokens;

        return reconstructed;
    }


    Session _session;
    std::string _url;
};

}// namespace flock
