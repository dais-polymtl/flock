#pragma once

#include "session.hpp"

#include "flockmtl/model_manager/providers/handlers/handler.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace flockmtl {

class AzureModelManager : public IModelProviderHandler {
public:
    AzureModelManager(std::string token, std::string resource_name, std::string deployment_model_name,
                      std::string api_version, bool throw_exception)
        : _token(token), _resource_name(resource_name), _deployment_model_name(deployment_model_name),
          _api_version(api_version), _session("Azure", throw_exception), _throw_exception(throw_exception) {
        _session.setToken(token, "");
    }

    AzureModelManager(const AzureModelManager&) = delete;
    AzureModelManager& operator=(const AzureModelManager&) = delete;
    AzureModelManager(AzureModelManager&&) = delete;
    AzureModelManager& operator=(AzureModelManager&&) = delete;

    nlohmann::json CallComplete(const nlohmann::json& json, const std::string& contentType = "application/json") {
        std::string url = "https://" + _resource_name + ".openai.azure.com/openai/deployments/" +
                          _deployment_model_name + "/chat/completions?api-version=" + _api_version;
        _session.setUrl(url);
        return execute_post(json.dump(), contentType);
    }

    nlohmann::json CallEmbedding(const nlohmann::json& json, const std::string& contentType = "application/json") {
        std::string url = "https://" + _resource_name + ".openai.azure.com/openai/deployments/" +
                          _deployment_model_name + "/embeddings?api-version=" + _api_version;
        _session.setUrl(url);
        return execute_post(json.dump(), contentType);
    }


private:
    std::string _token;
    std::string _resource_name;
    std::string _deployment_model_name;
    std::string _api_version;
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
            throw std::runtime_error("[Azure] error. Reason: " + msg);
        } else {
            std::cerr << "[Azure] error. Reason: " << msg << '\n';
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