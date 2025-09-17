#pragma once

#include "flock/model_manager/providers/handlers/handler.hpp"
#include "session.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace flock {

class BaseModelProviderHandler : public IModelProviderHandler {
public:
    explicit BaseModelProviderHandler(bool throw_exception)
        : _throw_exception(throw_exception) {}
    virtual ~BaseModelProviderHandler() = default;

    // AddRequest: just add the json to the batch (type is ignored, kept for interface compatibility)
    void AddRequest(const nlohmann::json& json, RequestType type = RequestType::Completion) {
        _request_batch.push_back(json);
    }

    // CollectCompletions: process all as completions, then clear
    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") {
        std::vector<nlohmann::json> completions;
        if (!_request_batch.empty()) completions = ExecuteBatch(_request_batch, true, contentType, true);
        _request_batch.clear();
        return completions;
    }

    // CollectEmbeddings: process all as embeddings, then clear
    std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") {
        std::vector<nlohmann::json> embeddings;
        if (!_request_batch.empty()) embeddings = ExecuteBatch(_request_batch, true, contentType, false);
        _request_batch.clear();
        return embeddings;
    }

    // Unified batch implementation with customizable headers
    std::vector<nlohmann::json> ExecuteBatch(const std::vector<nlohmann::json>& jsons, bool async = true, const std::string& contentType = "application/json", bool is_completion = true) {
        struct CurlRequestData {
            std::string response;
            CURL* easy = nullptr;
            std::string payload;
        };
        std::vector<CurlRequestData> requests(jsons.size());
        CURLM* multi_handle = curl_multi_init();
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        for (const auto& h: getExtraHeaders()) {
            headers = curl_slist_append(headers, h.c_str());
        }
        auto url = is_completion ? getCompletionUrl() : getEmbedUrl();
        for (size_t i = 0; i < jsons.size(); ++i) {
            requests[i].payload = jsons[i].dump();
            requests[i].easy = curl_easy_init();
            curl_easy_setopt(requests[i].easy, CURLOPT_URL, url.c_str());
            curl_easy_setopt(requests[i].easy, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(requests[i].easy, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                std::string* resp = static_cast<std::string*>(userdata);
                resp->append(ptr, size * nmemb);
                return size * nmemb; });
            curl_easy_setopt(requests[i].easy, CURLOPT_WRITEDATA, &requests[i].response);
            curl_easy_setopt(requests[i].easy, CURLOPT_POST, 1L);
            curl_easy_setopt(requests[i].easy, CURLOPT_POSTFIELDS, requests[i].payload.c_str());
            curl_multi_add_handle(multi_handle, requests[i].easy);
        }
        int still_running = 0;
        curl_multi_perform(multi_handle, &still_running);
        while (still_running) {
            int numfds;
            curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
            curl_multi_perform(multi_handle, &still_running);
        }
        std::vector<nlohmann::json> results(jsons.size());
        for (size_t i = 0; i < requests.size(); ++i) {
            curl_easy_getinfo(requests[i].easy, CURLINFO_RESPONSE_CODE, NULL);
            if (!requests[i].response.empty() && isJson(requests[i].response)) {
                try {
                    nlohmann::json parsed = nlohmann::json::parse(requests[i].response);
                    checkResponse(parsed, is_completion);
                    if (is_completion) {
                        results[i] = ExtractCompletionOutput(parsed);
                    } else {
                        results[i] = ExtractEmbeddingVector(parsed);
                    }
                } catch (const std::exception& e) {
                    trigger_error(std::string("JSON parse error: ") + e.what());
                }
            } else {
                trigger_error("Empty or invalid response in batch");
            }
            curl_multi_remove_handle(multi_handle, requests[i].easy);
            curl_easy_cleanup(requests[i].easy);
        }
        curl_slist_free_all(headers);
        curl_multi_cleanup(multi_handle);
        return results;
    }

    virtual void setParameters(const std::string& data, const std::string& contentType = "") = 0;
    virtual auto postRequest(const std::string& contentType) -> decltype(((Session*) nullptr)->postPrepare(contentType)) = 0;

protected:
    bool _throw_exception;
    std::vector<nlohmann::json> _request_batch;

    virtual std::string getCompletionUrl() const = 0;
    virtual std::string getEmbedUrl() const = 0;
    virtual void prepareSessionForRequest(const std::string& url) = 0;
    virtual std::vector<std::string> getExtraHeaders() const { return {}; }
    virtual void checkProviderSpecificResponse(const nlohmann::json&, bool is_completion) {}
    virtual nlohmann::json ExtractCompletionOutput(const nlohmann::json&) const { return {}; }
    virtual nlohmann::json ExtractEmbeddingVector(const nlohmann::json&) const { return {}; }

    void trigger_error(const std::string& msg) {
        if (_throw_exception) {
            throw std::runtime_error("[ModelProvider] error. Reason: " + msg);
        } else {
            std::cerr << "[ModelProvider] error. Reason: " << msg << '\n';
        }
    }

    void checkResponse(const nlohmann::json& json, bool is_completion) {
        if (json.contains("error")) {
            auto reason = json["error"].dump();
            trigger_error(reason);
            std::cerr << ">> response error :\n"
                      << json.dump(2) << "\n";
        }
        checkProviderSpecificResponse(json, is_completion);
    }

    bool isJson(const std::string& data) {
        try {
            nlohmann::json::parse(data);
        } catch (...) {
            return false;
        }
        return true;
    }
};

}// namespace flock
