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
    std::string getCompletionUrl() const override { return _url + "/api/generate"; }
    std::string getEmbedUrl() const override { return _url + "/api/embed"; }
    void prepareSessionForRequest(const std::string& url) override { _session.setUrl(url); }
    void setParameters(const std::string& data, const std::string& contentType = "") override {
        if (contentType != "multipart/form-data") {
            _session.setBody(data);
        }
    }
    auto postRequest(const std::string& contentType) -> decltype(((Session*) nullptr)->postPrepareOllama(contentType)) override {
        return _session.postPrepareOllama(contentType);
    }
    void checkProviderSpecificResponse(const nlohmann::json& response, bool is_completion) override {
        if (is_completion) {
            if ((response.contains("done_reason") && response["done_reason"] != "stop") ||
                (response.contains("done") && !response["done"].is_null() && response["done"].get<bool>() != true)) {
                throw std::runtime_error("The request was refused due to some internal error with Ollama API");
            }
        } else {
            if (response.contains("embeddings") && (!response["embeddings"].is_array() || response["embeddings"].empty())) {
                throw std::runtime_error("Ollama API returned empty or invalid embedding data.");
            }
        }
    }

    nlohmann::json ExtractCompletionOutput(const nlohmann::json& response) const override {
        if (response.contains("response")) {
            return nlohmann::json::parse(response["response"].get<std::string>());
        }
        return {};
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

    Session _session;
    std::string _url;
};

}// namespace flock
