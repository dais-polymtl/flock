#ifndef _FLOCK_MTL_MODEL_MANAGER_OLLAMA_H
#define _FLOCK_MTL_MODEL_MANAGER_OLLAMA_H

#include "session.hpp"

#include "flockmtl/model_manager/providers/handlers/handler.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace flockmtl {

class OllamaModelManager : public IModelProviderHandler {
public:
    OllamaModelManager(const std::string& url, const bool throw_exception)
        : _session("Ollama", throw_exception), _throw_exception(throw_exception), _url(url) {}
    OllamaModelManager(const OllamaModelManager&) = delete;
    OllamaModelManager& operator=(const OllamaModelManager&) = delete;
    OllamaModelManager(OllamaModelManager&&) = delete;
    OllamaModelManager& operator=(OllamaModelManager&&) = delete;

    std::string GetChatUrl() const { return _url + "/api/generate"; }

    std::string GetEmbedUrl() const { return _url + "/api/embeddings"; }

    std::string GetAvailableOllamaModelsUrl() {
        static int check_done = -1;
        static const char* url = nullptr;

        if (check_done == -1) {
            url = std::getenv("OLLAMA_AVAILABLE_MODELS_URL");
            check_done = 1;
        }

        if (!url) {
            throw std::runtime_error("OLLAMA_AVAILABLE_MODELS_URL environment variable is not set.");
        }

        return url;
    }

    nlohmann::json CallComplete(const nlohmann::json& json, const std::string& contentType = "application/json") {
        const std::string url = GetChatUrl();
        _session.setUrl(url);
        return execute_post(json.dump(), contentType);
    }

    nlohmann::json CallEmbedding(const nlohmann::json& json, const std::string& contentType = "application/json") {
        const std::string url = GetEmbedUrl();
        _session.setUrl(url);
        return execute_post(json.dump(), contentType);
    }

    bool validModel(const std::string& user_model_name) {
        std::string url = GetAvailableOllamaModelsUrl();
        auto response = _session.validOllamaModelsJson(url);
        auto json = nlohmann::json::parse(response.text);
        bool res = false;
        for (const auto& model: json["models"]) {
            if (model.contains("name")) {
                const auto& available_model = model["name"].get<std::string>();
                res |= available_model.find(user_model_name) != std::string::npos;
            }
        }
        return res;
    }

private:
    Session _session;
    bool _throw_exception;
    std::string _url;

    nlohmann::json execute_post(const std::string& data, const std::string& contentType) {
        setParameters(data, contentType);
        auto response = _session.postPrepareOllama(contentType);
        if (response.is_error) {
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
            throw std::runtime_error(msg);
        } else {
            std::cerr << "[Ollama] error. Reason: " << msg << '\n';
        }
    }

    void checkResponse(const nlohmann::json& json) {
        if (json.count("error")) {
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
#endif