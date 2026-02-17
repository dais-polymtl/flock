#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "session.hpp"
#include <cstdlib>

namespace flock {

class OpenAIModelManager : public BaseModelProviderHandler {
public:
    OpenAIModelManager(std::string token, std::string api_base_url, bool throw_exception)
        : BaseModelProviderHandler(throw_exception), _token(token), _session("OpenAI", throw_exception) {
        _session.setToken(token, "");
        if (api_base_url.empty()) {
            _api_base_url = "https://api.openai.com/v1/";
        } else {
            _api_base_url = api_base_url + '/';
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
    std::string getTranscriptionUrl() const override {
        return _api_base_url + "audio/transcriptions";
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
    void checkProviderSpecificResponse(const nlohmann::json& response, RequestType request_type) override {
        if (request_type == RequestType::Transcription) {
            return;// No specific checks needed for transcriptions
        }
        bool is_completion = (request_type == RequestType::Completion);
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

    std::pair<int64_t, int64_t> ExtractTokenUsage(const nlohmann::json& response) const override {
        int64_t input_tokens = 0;
        int64_t output_tokens = 0;
        if (response.contains("usage") && response["usage"].is_object()) {
            const auto& usage = response["usage"];
            if (usage.contains("prompt_tokens") && usage["prompt_tokens"].is_number()) {
                input_tokens = usage["prompt_tokens"].get<int64_t>();
            }
            if (usage.contains("completion_tokens") && usage["completion_tokens"].is_number()) {
                output_tokens = usage["completion_tokens"].get<int64_t>();
            }
        }
        return {input_tokens, output_tokens};
    }


    nlohmann::json ExtractTranscriptionOutput(const nlohmann::json& response) const override {
        // Transcription API returns JSON with "text" field when response_format=json
        if (response.contains("text") && !response["text"].is_null()) {
            return response["text"].get<std::string>();
        }
        return "";
    }
};

}// namespace flock
