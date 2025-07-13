
#pragma once

#include "flockmtl/model_manager/providers/handlers/handler.hpp"
#include "session.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace flockmtl {

class OpenAIModelManager : public IModelProviderHandler {
public:
    OpenAIModelManager(std::string token, std::string api_base_url, bool throw_exception)
        : _token(token), _session("OpenAI", throw_exception), _throw_exception(throw_exception) {
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

    nlohmann::json CallComplete(const nlohmann::json& json, const std::string& contentType = "application/json") {
        std::string url = _api_base_url + "chat/completions";
        _session.setUrl(url);
        return execute_post(json.dump(), contentType);
    }

    nlohmann::json CallEmbedding(const nlohmann::json& json, const std::string& contentType = "application/json") {
        std::string url = _api_base_url + "embeddings";
        _session.setUrl(url);
        return execute_post(json.dump(), contentType);
    }


private:
    std::string _token;
    std::string _api_base_url;
    Session _session;
    bool _throw_exception;

    nlohmann::json execute_post(const std::string& data, const std::string& contentType) {
        setParameters(data, contentType);
        auto response = _session.postPrepare(contentType);
        if (response.is_error) {
            std::cout << ">> response error :\n"
                      << response.text << "\n";
            trigger_error(response.error_message);
        }

        nlohmann::json json{};
        if (isJson(response.text)) {
            json = nlohmann::json::parse(response.text);
            checkResponse(json);
        } else {
            trigger_error("Response is not a valid JSON");
        }

        return json;
    }

    void trigger_error(const std::string& msg) {
        if (_throw_exception) {
            throw std::runtime_error("[OpenAI] error. Reason: " + msg);
        } else {
            std::cerr << "[OpenAI] error. Reason: " << msg << '\n';
        }
    }

    void checkResponse(const nlohmann::json& json) {
        if (json.contains("error")) {
            auto reason = json["error"].dump();
            trigger_error(reason);
            std::cerr << ">> response error :\n"
                      << json.dump(2) << "\n";
        }
    }

    bool isJson(const std::string& data) {
        bool rc = true;
        try {
            auto json = nlohmann::json::parse(data);// throws if no json
        } catch (std::exception&) {
            rc = false;
        }
        return (rc);
    }

    void setParameters(const std::string& data, const std::string& contentType = "") {
        if (contentType != "multipart/form-data") {
            _session.setBody(data);
        }
    }
};

}// namespace flockmtl
