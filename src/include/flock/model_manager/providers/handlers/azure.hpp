#include "flock/model_manager/providers/handlers/base_handler.hpp"

namespace flock {

class AzureModelManager : public BaseModelProviderHandler {
public:
    AzureModelManager(std::string token, std::string resource_name, std::string deployment_model_name,
                      std::string api_version, bool throw_exception)
        : BaseModelProviderHandler(throw_exception),
          _token(token), _resource_name(resource_name), _deployment_model_name(deployment_model_name),
          _api_version(api_version), _session("Azure", throw_exception) {
        _session.setToken(token, "");
    }

    AzureModelManager(const AzureModelManager&) = delete;
    AzureModelManager& operator=(const AzureModelManager&) = delete;
    AzureModelManager(AzureModelManager&&) = delete;
    AzureModelManager& operator=(AzureModelManager&&) = delete;

protected:
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
                        throw std::runtime_error("Azure API did not finish successfully. finish_reason: " + finish_reason);
                    }
                }
            }
        } else {
            if (response.contains("data") && response["data"].is_array() && response["data"].empty()) {
                throw std::runtime_error("Azure API returned empty embedding data.");
            }
        }
    }
    std::string getCompletionUrl() const override {
        return "https://" + _resource_name + ".openai.azure.com/openai/deployments/" +
               _deployment_model_name + "/chat/completions?api-version=" + _api_version;
    }
    std::string getEmbedUrl() const override {
        return "https://" + _resource_name + ".openai.azure.com/openai/deployments/" +
               _deployment_model_name + "/embeddings?api-version=" + _api_version;
    }
    std::string getTranscriptionUrl() const override {
        return "https://" + _resource_name + ".openai.azure.com/openai/deployments/" +
               _deployment_model_name + "/audio/transcriptions?api-version=" + _api_version;
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

    nlohmann::json ExtractCompletionOutput(const nlohmann::json& response) const override {
        if (response.contains("choices") && response["choices"].is_array() && !response["choices"].empty()) {
            const auto& choice = response["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                return choice["message"]["content"].get<std::string>();
            }
        }
        return {};
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


    std::string _token;
    std::string _resource_name;
    std::string _deployment_model_name;
    std::string _api_version;
    Session _session;
};

}// namespace flock
