#pragma once

#include "flock/core/common.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/providers/handlers/handler.hpp"
#include "session.hpp"
#include <cstdio>
#include <curl/curl.h>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace flock {

class BaseModelProviderHandler : public IModelProviderHandler {
public:
    explicit BaseModelProviderHandler(bool throw_exception)
        : _throw_exception(throw_exception) {}
    virtual ~BaseModelProviderHandler() = default;

    void AddRequest(const nlohmann::json& json, RequestType type = RequestType::Completion) override {
        _request_batch.push_back(json);
        _request_types.push_back(type);
    }

    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") override {
        std::vector<nlohmann::json> completions;
        if (!_request_batch.empty()) completions = ExecuteBatch(_request_batch, true, contentType, RequestType::Completion);
        _request_batch.clear();
        return completions;
    }

    std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") override {
        std::vector<nlohmann::json> embeddings;
        if (!_request_batch.empty()) embeddings = ExecuteBatch(_request_batch, true, contentType, RequestType::Embedding);
        _request_batch.clear();
        return embeddings;
    }


    std::vector<nlohmann::json> CollectTranscriptions(const std::string& contentType = "multipart/form-data") override {
        std::vector<nlohmann::json> transcriptions;
        if (!_request_batch.empty()) {
            std::vector<nlohmann::json> transcription_batch;
            for (size_t i = 0; i < _request_batch.size(); ++i) {
                if (_request_types[i] == RequestType::Transcription) {
                    transcription_batch.push_back(_request_batch[i]);
                }
            }

            if (!transcription_batch.empty()) {
                transcriptions = ExecuteBatch(transcription_batch, true, contentType, RequestType::Transcription);
                // Remove transcription requests from batch
                for (size_t i = _request_batch.size(); i > 0; --i) {
                    if (_request_types[i - 1] == RequestType::Transcription) {
                        _request_batch.erase(_request_batch.begin() + i - 1);
                        _request_types.erase(_request_types.begin() + i - 1);
                    }
                }
            }
        }
        return transcriptions;
    }


public:
protected:
    std::vector<nlohmann::json> ExecuteBatch(const std::vector<nlohmann::json>& jsons, bool async = true, const std::string& contentType = "application/json", RequestType request_type = RequestType::Completion) {
#ifdef __EMSCRIPTEN__
        // WASM: Process requests sequentially using emscripten fetch
        std::vector<nlohmann::json> results(jsons.size());
        bool is_completion = (request_type == RequestType::Completion);
        auto url = is_completion ? getCompletionUrl() : getEmbedUrl();

        for (size_t i = 0; i < jsons.size(); ++i) {
            prepareSessionForRequest(url);
            setParameters(jsons[i].dump(), contentType);
            auto response = postRequest(contentType);

            if (!response.is_error && !response.text.empty() && isJson(response.text)) {
                try {
                    nlohmann::json parsed = nlohmann::json::parse(response.text);
                    checkResponse(parsed, request_type);
                    if (is_completion) {
                        results[i] = ExtractCompletionOutput(parsed);
                    } else {
                        results[i] = ExtractEmbeddingVector(parsed);
                    }
                } catch (const std::exception& e) {
                    trigger_error(std::string("JSON parse error: ") + e.what());
                }
            } else {
                trigger_error("Empty or invalid response: " + response.error_message);
            }
        }
        return results;
#else
        // Native: Use curl multi-handle for parallel requests
        struct CurlRequestData {
            std::string response;
            CURL* easy = nullptr;
            std::string payload;
            curl_mime* mime_form = nullptr;
            std::string temp_file_path;
            bool is_temp_file;
        };
        std::vector<CurlRequestData> requests(jsons.size());
        CURLM* multi_handle = curl_multi_init();

        // Determine URL based on request type
        std::string url;
        bool is_transcription = (request_type == RequestType::Transcription);
        bool is_completion = (request_type == RequestType::Completion);
        if (is_transcription) {
            url = getTranscriptionUrl();
        } else if (is_completion) {
            url = getCompletionUrl();
        } else {
            url = getEmbedUrl();
        }

        // Prepare all requests
        for (size_t i = 0; i < jsons.size(); ++i) {
            requests[i].easy = curl_easy_init();
            curl_easy_setopt(requests[i].easy, CURLOPT_URL, url.c_str());

            if (is_transcription) {
                // Handle transcription requests (multipart/form-data)
                const auto& req = jsons[i];
                if (!req.contains("file_path") || req["file_path"].is_null()) {
                    trigger_error("Missing or null file_path in transcription request");
                }
                if (!req.contains("model") || req["model"].is_null()) {
                    trigger_error("Missing or null model in transcription request");
                }
                auto file_path = req["file_path"].get<std::string>();
                auto model = req["model"].get<std::string>();
                auto prompt = req.contains("prompt") && !req["prompt"].is_null() ? req["prompt"].get<std::string>() : "";
                requests[i].is_temp_file = req.contains("is_temp_file") ? req["is_temp_file"].get<bool>() : false;
                if (requests[i].is_temp_file) {
                    requests[i].temp_file_path = file_path;
                }

                // Set up multipart form data
                requests[i].mime_form = curl_mime_init(requests[i].easy);
                curl_mimepart* field = curl_mime_addpart(requests[i].mime_form);
                curl_mime_name(field, "file");
                curl_mime_filedata(field, file_path.c_str());

                field = curl_mime_addpart(requests[i].mime_form);
                curl_mime_name(field, "model");
                curl_mime_data(field, model.c_str(), CURL_ZERO_TERMINATED);

                field = curl_mime_addpart(requests[i].mime_form);
                curl_mime_name(field, "response_format");
                curl_mime_data(field, "json", CURL_ZERO_TERMINATED);

                if (!prompt.empty()) {
                    field = curl_mime_addpart(requests[i].mime_form);
                    curl_mime_name(field, "prompt");
                    curl_mime_data(field, prompt.c_str(), CURL_ZERO_TERMINATED);
                }

                curl_easy_setopt(requests[i].easy, CURLOPT_MIMEPOST, requests[i].mime_form);

                // Set headers
                struct curl_slist* headers = nullptr;
                headers = curl_slist_append(headers, "Expect:");
                for (const auto& h: getExtraHeaders()) {
                    headers = curl_slist_append(headers, h.c_str());
                }
                curl_easy_setopt(requests[i].easy, CURLOPT_HTTPHEADER, headers);
            } else {
                // Handle JSON requests (completions/embeddings)
                requests[i].payload = jsons[i].dump();
                struct curl_slist* headers = nullptr;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                for (const auto& h: getExtraHeaders()) {
                    headers = curl_slist_append(headers, h.c_str());
                }
                curl_easy_setopt(requests[i].easy, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(requests[i].easy, CURLOPT_POST, 1L);
                curl_easy_setopt(requests[i].easy, CURLOPT_POSTFIELDS, requests[i].payload.c_str());
            }

            // Set response callback
            curl_easy_setopt(
                    requests[i].easy, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                std::string* resp = static_cast<std::string*>(userdata);
                resp->append(ptr, size * nmemb);
                return size * nmemb; });
            curl_easy_setopt(requests[i].easy, CURLOPT_WRITEDATA, &requests[i].response);

            curl_multi_add_handle(multi_handle, requests[i].easy);
        }

        auto api_start = std::chrono::high_resolution_clock::now();

        int still_running = 0;
        curl_multi_perform(multi_handle, &still_running);
        while (still_running) {
            int numfds;
            curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
            curl_multi_perform(multi_handle, &still_running);
        }

        auto api_end = std::chrono::high_resolution_clock::now();
        double api_duration_ms = std::chrono::duration<double, std::milli>(api_end - api_start).count();

        int64_t batch_input_tokens = 0;
        int64_t batch_output_tokens = 0;

        std::vector<nlohmann::json> results(jsons.size());
        for (size_t i = 0; i < requests.size(); ++i) {
            // Clean up temp files for transcriptions
            if (is_transcription && requests[i].is_temp_file && !requests[i].temp_file_path.empty()) {
                std::remove(requests[i].temp_file_path.c_str());
            }

            curl_easy_getinfo(requests[i].easy, CURLINFO_RESPONSE_CODE, NULL);

            if (isJson(requests[i].response)) {
                try {
                    nlohmann::json parsed = nlohmann::json::parse(requests[i].response);
                    checkResponse(parsed, request_type);

                    // Extract token usage for completions/embeddings
                    if (!is_transcription) {
                        auto [input_tokens, output_tokens] = ExtractTokenUsage(parsed);
                        batch_input_tokens += input_tokens;
                        batch_output_tokens += output_tokens;
                    }

                    // Let provider extract output based on request type
                    try {
                        results[i] = ExtractOutput(parsed, request_type);
                    } catch (const std::exception& e) {
                        trigger_error(std::string("Output extraction error: ") + e.what());
                    }
                } catch (const std::exception& e) {
                    trigger_error(std::string("Response processing error: ") + e.what());
                }
            } else {
                trigger_error("Invalid JSON response: " + requests[i].response);
            }

            // Clean up mime form for transcriptions
            if (is_transcription && requests[i].mime_form) {
                curl_mime_free(requests[i].mime_form);
            }
            curl_multi_remove_handle(multi_handle, requests[i].easy);
            curl_easy_cleanup(requests[i].easy);
        }

        if (!is_transcription) {
            MetricsManager::UpdateTokens(batch_input_tokens, batch_output_tokens);
        }
        MetricsManager::AddApiDuration(api_duration_ms);
        for (size_t i = 0; i < jsons.size(); ++i) {
            MetricsManager::IncrementApiCalls();
        }

        curl_multi_cleanup(multi_handle);
        return results;
#endif
    }

    virtual void setParameters(const std::string& data, const std::string& contentType = "") = 0;
    virtual auto postRequest(const std::string& contentType) -> decltype(((Session*) nullptr)->postPrepare(contentType)) = 0;

protected:
    bool _throw_exception;
    std::vector<nlohmann::json> _request_batch;
    std::vector<RequestType> _request_types;

    virtual std::string getCompletionUrl() const = 0;
    virtual std::string getEmbedUrl() const = 0;
    virtual std::string getTranscriptionUrl() const = 0;
    virtual void prepareSessionForRequest(const std::string& url) = 0;
    virtual std::vector<std::string> getExtraHeaders() const { return {}; }
    virtual void checkProviderSpecificResponse(const nlohmann::json&, RequestType request_type) {}
    virtual nlohmann::json ExtractCompletionOutput(const nlohmann::json&) const { return {}; }
    virtual nlohmann::json ExtractEmbeddingVector(const nlohmann::json&) const { return {}; }
    virtual nlohmann::json ExtractTranscriptionOutput(const nlohmann::json&) const = 0;

    // Unified extraction method - delegates to specific Extract* methods based on request type
    nlohmann::json ExtractOutput(const nlohmann::json& parsed, RequestType request_type) const {
        if (request_type == RequestType::Completion) {
            return ExtractCompletionOutput(parsed);
        } else if (request_type == RequestType::Embedding) {
            return ExtractEmbeddingVector(parsed);
        } else {
            return ExtractTranscriptionOutput(parsed);
        }
    }
    virtual std::pair<int64_t, int64_t> ExtractTokenUsage(const nlohmann::json& response) const = 0;

    void trigger_error(const std::string& msg) {
        if (_throw_exception) {
            throw std::runtime_error("[ModelProvider] error. Reason: " + msg);
        } else {
            std::cerr << "[ModelProvider] error. Reason: " << msg << '\n';
        }
    }

    void checkResponse(const nlohmann::json& json, RequestType request_type) {
        if (json.contains("error")) {
            auto reason = json["error"].dump();
            trigger_error(reason);
            std::cerr << ">> response error :\n"
                      << json.dump(2) << "\n";
        }
        checkProviderSpecificResponse(json, request_type);
    }

    bool isJson(const std::string& data) {
        try {
            (void)nlohmann::json::parse(data);
        } catch (...) {
            return false;
        }
        return true;
    }
};

}// namespace flock
