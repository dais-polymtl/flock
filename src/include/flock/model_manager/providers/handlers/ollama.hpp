#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "session.hpp"
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

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
            // Embedding-specific checks (if any) can be added here
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

    Session _session;
    std::string _url;
};

}// namespace flock
