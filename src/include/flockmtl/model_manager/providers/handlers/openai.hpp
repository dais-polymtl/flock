#pragma once

#include "flockmtl/model_manager/providers/handlers/base_handler.hpp"
#include "session.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace flockmtl {

class OpenAIModelManager : public BaseModelProviderHandler {
public:
    OpenAIModelManager(std::string token, std::string api_base_url, bool throw_exception)
        : BaseModelProviderHandler(throw_exception), _token(token), _session("OpenAI", throw_exception) {
        _session.setToken(token, "");
        if (api_base_url.empty()) {
            _api_base_url = "https://api.openai.com/v1/";
        } else {
            _api_base_url = api_base_url;
        }
        _session.setUrl(_api_base_url);
    }

    OpenAIModelManager(const OpenAIModelManager&) = delete;
    OpenAIModelManager& operator=(const OpenAIModelManager&) = delete;
    OpenAIModelManager(OpenAIModelManager&&) = delete;
    OpenAIModelManager& operator=(OpenAIModelManager&&) = delete;

protected:
    std::string _token;
    std::string _api_base_url;
    Session _session;

    std::string getCompletionUrl() const override {
        return _api_base_url + "chat/completions";
    }
    std::string getEmbedUrl() const override {
        return _api_base_url + "embeddings";
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
        return {"Authorization: Bearer " + _token};
    }
    void checkProviderSpecificResponse(const nlohmann::json& response, bool is_completion) override {
        if (is_completion) {
            if (response.contains("choices") && response["choices"].is_array() && !response["choices"].empty()) {
                const auto& choice = response["choices"][0];
                if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                    std::string finish_reason = choice["finish_reason"].get<std::string>();
                    if (finish_reason != "stop" && finish_reason != "length") {
                        throw std::runtime_error("OpenAI API did not finish successfully. finish_reason: " + finish_reason);
                    }
                }
            }
        } else {
            // Embedding-specific checks (if any) can be added here
            if (response.contains("data") && response["data"].is_array() && response["data"].empty()) {
                throw std::runtime_error("OpenAI API returned empty embedding data.");
            }
        }
    }
    nlohmann::json ExtractCompletionOutput(const nlohmann::json& response) const override {
        if (response.contains("choices") && response["choices"].is_array() && !response["choices"].empty()) {
            const auto& choice = response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                return nlohmann::json::parse(choice["message"]["content"].get<std::string>());
            }
        }
        return {};
    }

    nlohmann::json ExtractEmbeddingVector(const nlohmann::json& response) const override {
        auto results = nlohmann::json::array();
        if (response.contains("data") && response["data"].is_array() && !response["data"].empty()) {
            const auto& embeddings = response["data"];
            for (const auto& embedding: embeddings) {
                results.push_back(embedding["embedding"]);
            }
            return results;
        }
    }
};

}// namespace flockmtl
