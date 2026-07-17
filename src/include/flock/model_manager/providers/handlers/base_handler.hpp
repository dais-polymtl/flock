#pragma once

#include "flock/core/common.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/providers/handlers/handler.hpp"
#include "flock/model_manager/providers/provider.hpp"
#include "flock/model_manager/rate_limiter.hpp"
#include "flock/model_manager/usage_limiter.hpp"
#include "session.hpp"
#include <cstdio>
#include <curl/curl.h>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace flock {

class BaseModelProviderHandler : public IModelProviderHandler {
public:
    explicit BaseModelProviderHandler(bool throw_exception, const std::string& model_name = "",
                                      std::optional<int> rate_limit = std::nullopt,
                                      std::optional<UsageLimit> usage_limit = std::nullopt,
                                      std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                                      std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : _throw_exception(throw_exception),
          _model_name(model_name),
          _rate_limit(rate_limit),
          _usage_limit(std::move(usage_limit)),
          _rate_limiter(std::move(rate_limiter)),
          _usage_limiter(std::move(usage_limiter)) {}
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
        ThrowOnTokenLimitMarkers(embeddings);
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
                ThrowOnTokenLimitMarkers(transcriptions);
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
        if (_rate_limit.has_value() && _rate_limiter != nullptr) {
            _rate_limiter->WaitForBatchIfNeeded(jsons.size(), static_cast<size_t>(_rate_limit.value()));
        }

        EnsureUsageLimitNotExceeded();

        // Detect streaming: check if the first completion request has "stream": true
        bool is_streamed = false;
        if (request_type == RequestType::Completion) {
            for (const auto& json: jsons) {
                if (json.contains("stream") && json["stream"].is_boolean() && json["stream"].get<bool>()) {
                    is_streamed = true;
                    break;
                }
            }
        }
        if (is_streamed) {
            return ExecuteBatchStreamed(jsons, contentType);
        }

#ifdef __EMSCRIPTEN__
        // WASM: Process requests sequentially using emscripten fetch
        std::vector<nlohmann::json> results(jsons.size());
        bool is_completion = (request_type == RequestType::Completion);
        bool is_transcription = (request_type == RequestType::Transcription);
        auto url = is_completion ? getCompletionUrl() : getEmbedUrl();
        bool usage_limit_reached = false;

        for (size_t i = 0; i < jsons.size(); ++i) {
            EnsureUsageLimitNotExceeded();

            prepareSessionForRequest(url);
            setParameters(jsons[i].dump(), contentType);
            auto response = postRequest(contentType);

            if (!response.is_error && !response.text.empty() && isJson(response.text)) {
                try {
                    nlohmann::json parsed = nlohmann::json::parse(response.text);
                    checkResponse(parsed, request_type);
                    if (!is_transcription) {
                        auto [input_tokens, output_tokens] = ExtractTokenUsage(parsed);
                        RecordTokenUsageWithSoftCap(input_tokens, output_tokens, usage_limit_reached);
                    }
                    ExtractOutputWithErrorHandling(parsed, request_type, results[i]);
                } catch (const TokenLimitExceededError&) {
                    results[i] = TokenLimitExceededMarker();
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
        bool usage_limit_reached = false;
        for (size_t i = 0; i < requests.size(); ++i) {
            // Clean up temp files for transcriptions
            if (is_transcription && requests[i].is_temp_file && !requests[i].temp_file_path.empty()) {
                std::remove(requests[i].temp_file_path.c_str());
            }

            long http_code = 0;
            curl_easy_getinfo(requests[i].easy, CURLINFO_RESPONSE_CODE, &http_code);

            if (requests[i].response.empty()) {
                trigger_error("Empty response from provider (HTTP " + std::to_string(http_code) + ", URL: " + url + ")");
            } else if (isJson(requests[i].response)) {
                try {
                    nlohmann::json parsed = nlohmann::json::parse(requests[i].response);
                    checkResponse(parsed, request_type);

                    // Extract token usage for completions/embeddings
                    if (!is_transcription) {
                        auto [input_tokens, output_tokens] = ExtractTokenUsage(parsed);
                        batch_input_tokens += input_tokens;
                        batch_output_tokens += output_tokens;
                        RecordTokenUsageWithSoftCap(input_tokens, output_tokens, usage_limit_reached);
                    }

                    ExtractOutputWithErrorHandling(parsed, request_type, results[i]);
                } catch (const TokenLimitExceededError&) {
                    results[i] = TokenLimitExceededMarker();
                } catch (const nlohmann::json::exception& e) {
                    trigger_error(std::string("Response processing error: ") + e.what());
                }
            } else {
                trigger_error("Invalid JSON response (HTTP " + std::to_string(http_code) + ", URL: " + url + "): " + requests[i].response);
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

    // Streaming execution path for SSE responses
    std::vector<nlohmann::json> ExecuteBatchStreamed(const std::vector<nlohmann::json>& jsons, const std::string& contentType = "application/json") {
#ifdef __EMSCRIPTEN__
        // WASM: Process streaming requests sequentially
        std::vector<nlohmann::json> results(jsons.size());
        for (size_t i = 0; i < jsons.size(); ++i) {
            prepareSessionForRequest(getCompletionUrl());
            setParameters(jsons[i].dump(), contentType);
            auto response = postRequest(contentType);

            if (response.is_error || response.text.empty()) {
                trigger_error(response.error_message);
            } else {
                auto reconstructed = ReconstructFromStreamedChunks(response.text);
                if (reconstructed.is_null()) {
                    trigger_error("Failed to reconstruct streaming response");
                } else {
                    try {
                        checkResponse(reconstructed, RequestType::Completion);
                        results[i] = ExtractOutput(reconstructed, RequestType::Completion);
                    } catch (const TokenLimitExceededError&) {
                        results[i] = TokenLimitExceededMarker();
                    } catch (const std::exception& e) {
                        trigger_error(std::string("Response processing error: ") + e.what());
                    }
                }
            }
        }
        return results;
#else
        // Native: Use curl multi-handle with SSE accumulation
        struct CurlRequestData {
            std::string response;
            CURL* easy = nullptr;
            std::string payload;
        };
        std::vector<CurlRequestData> requests(jsons.size());
        CURLM* multi_handle = curl_multi_init();
        std::string url = getCompletionUrl();

        for (size_t i = 0; i < jsons.size(); ++i) {
            requests[i].easy = curl_easy_init();
            curl_easy_setopt(requests[i].easy, CURLOPT_URL, url.c_str());
            requests[i].payload = jsons[i].dump();
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            for (const auto& h: getExtraHeaders()) {
                headers = curl_slist_append(headers, h.c_str());
            }
            headers = curl_slist_append(headers, "Accept: text/event-stream");
            curl_easy_setopt(requests[i].easy, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(requests[i].easy, CURLOPT_POST, 1L);
            curl_easy_setopt(requests[i].easy, CURLOPT_POSTFIELDS, requests[i].payload.c_str());

            curl_easy_setopt(
                    requests[i].easy, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                        std::string* resp = static_cast<std::string*>(userdata);
                        resp->append(ptr, size * nmemb);
                        return size * nmemb;
                    });
            curl_easy_setopt(requests[i].easy, CURLOPT_WRITEDATA, &requests[i].response);
            curl_easy_setopt(requests[i].easy, CURLOPT_TIMEOUT, 600L);

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
            long http_code = 0;
            curl_easy_getinfo(requests[i].easy, CURLINFO_RESPONSE_CODE, &http_code);

            if (requests[i].response.empty()) {
                trigger_error("Empty streaming response from provider (HTTP " + std::to_string(http_code) + ", URL: " + url + ")");
            } else {
                auto reconstructed = ReconstructFromStreamedChunks(requests[i].response);
                if (reconstructed.is_null()) {
                    trigger_error("Failed to parse streaming response (HTTP " + std::to_string(http_code) + ")");
                } else {
                    try {
                        checkResponse(reconstructed, RequestType::Completion);
                        auto [input_tokens, output_tokens] = ExtractTokenUsage(reconstructed);
                        batch_input_tokens += input_tokens;
                        batch_output_tokens += output_tokens;
                        try {
                            results[i] = ExtractOutput(reconstructed, RequestType::Completion);
                        } catch (const std::exception& e) {
                            std::string msg = e.what();
                            if (msg.rfind("[ModelProvider]", 0) == 0) {
                                throw;
                            }
                            trigger_error(std::string("Output extraction error: ") + msg);
                        }
                    } catch (const TokenLimitExceededError&) {
                        results[i] = TokenLimitExceededMarker();
                    } catch (const std::exception& e) {
                        std::string msg = e.what();
                        if (msg.rfind("[ModelProvider]", 0) == 0) {
                            throw;
                        }
                        trigger_error(std::string("Streaming response processing error: ") + msg);
                    }
                }
            }
            curl_easy_cleanup(requests[i].easy);
        }

        MetricsManager::UpdateTokens(batch_input_tokens, batch_output_tokens);
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
    std::string _model_name;
    std::optional<int> _rate_limit;
    std::optional<UsageLimit> _usage_limit;
    std::shared_ptr<ModelRateLimiter> _rate_limiter;
    std::shared_ptr<ModelUsageLimiter> _usage_limiter;
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

    // Reconstruct a full completion JSON from raw SSE streaming text.
    // Returns nlohmann::json() (null) on failure on failure.
    virtual nlohmann::json ReconstructFromStreamedChunks(const std::string& sse_raw) const {
        // Default: no streaming support
        (void) sse_raw;
        return nlohmann::json();
    }

    // Shared utility: Reconstruct OpenAI-compatible completion JSON from SSE chunks.
    // Handles the standard format with content in delta.content.
    // Returns the reconstructed JSON in OpenAI non-streaming format shape.
    static nlohmann::json ReconstructOpenAIStreamingChunks(const std::string& sse_raw) {
        std::string accumulated_content;
        std::string finish_reason;
        nlohmann::json usage;

        std::istringstream stream(sse_raw);
        std::string line;

        while (std::getline(stream, line)) {
            // Trim whitespace
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();

            if (line.rfind("data: ", 0) == 0) {
                std::string json_str = line.substr(6);
                if (json_str.empty() || json_str == "[DONE]") continue;
                if (json_str[0] != '{' && json_str[0] != '[') continue;

                nlohmann::json chunk;
                try {
                    chunk = nlohmann::json::parse(json_str);
                } catch (...) {
                    continue;
                }

                // Accumulate delta content from choices.
                // Standard OpenAI: content is in delta.content.
                // vLLM: may send content="" on first chunk, content with answer on last chunk.
                // Reasoning (thinking) is always in delta.reasoning and should NOT be mixed into the final answer.
                if (chunk.contains("choices") && chunk["choices"].is_array()) {
                    for (const auto& choice: chunk["choices"]) {
                        if (choice.contains("delta") && choice["delta"].is_object()) {
                            auto& delta = choice["delta"];
                            if (delta.contains("content") && delta["content"].is_string()) {
                                std::string c = delta["content"].get<std::string>();
                                if (!c.empty()) {
                                    accumulated_content += c;
                                }
                            }
                            // Note: we intentionally DO NOT accumulate delta.reasoning.
                            // Reasoning is the model's thinking process and should be excluded from the final answer.
                        }
                        // Capture finish_reason from the last chunk that has it
                        if (choice.contains("finish_reason") && choice["finish_reason"].is_string()) {
                            finish_reason = choice["finish_reason"].get<std::string>();
                        }
                    }
                }

                // Capture usage - usually in the last chunk (can be at top level)
                if (chunk.contains("usage")) {
                    usage = chunk["usage"];
                }
            }
        }

        // Build the reconstructed JSON in the same shape as non-streaming response
        nlohmann::json message;
        message["role"] = "assistant";
        message["content"] = accumulated_content;
        nlohmann::json choice;
        choice["index"] = 0;
        choice["message"] = message;
        if (!finish_reason.empty()) choice["finish_reason"] = finish_reason;
        nlohmann::json reconstructed = {
                {"choices", nlohmann::json::array({choice})}};

        if (!usage.empty()) {
            reconstructed["usage"] = usage;
        }

        return reconstructed;
    }

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

    void RecordTokenUsage(int64_t prompt_tokens, int64_t completion_tokens) {
        if (_usage_limit.has_value() && _usage_limiter != nullptr) {
            _usage_limiter->RecordUsage(prompt_tokens, completion_tokens, _usage_limit);
        }
    }

    void EnsureUsageLimitNotExceeded() const {
        if (_usage_limit.has_value() && _usage_limiter != nullptr) {
            _usage_limiter->ThrowIfLimitExceeded(*_usage_limit);
        }
    }

    void RecordTokenUsageWithSoftCap(int64_t prompt_tokens, int64_t completion_tokens, bool& usage_limit_reached) {
        if (usage_limit_reached) {
            return;
        }
        try {
            RecordTokenUsage(prompt_tokens, completion_tokens);
        } catch (const UsageLimitExceededError&) {
            usage_limit_reached = true;
        }
    }

    void ExtractOutputWithErrorHandling(const nlohmann::json& parsed, RequestType request_type,
                                        nlohmann::json& result) {
        try {
            result = ExtractOutput(parsed, request_type);
        } catch (const std::exception& e) {
            std::string msg = e.what();
            if (msg.rfind("[ModelProvider]", 0) == 0) {
                throw;
            }
            trigger_error(std::string("Output extraction error: ") + msg);
        }
    }

    static void ThrowOnTokenLimitMarkers(const std::vector<nlohmann::json>& results) {
        for (const auto& result: results) {
            if (IsTokenLimitExceededMarker(result)) {
                throw TokenLimitExceededError();
            }
        }
    }

    void trigger_error(const std::string& msg) {
        const std::string prefix = "[ModelProvider] ";
        std::string full_message;
        if (msg.rfind(prefix, 0) == 0) {
            full_message = msg;
        } else {
            full_message = prefix + msg;
        }

        if (_throw_exception) {
            throw std::runtime_error(full_message);
        } else {
            std::cerr << full_message << '\n';
        }
    }

    void checkResponse(const nlohmann::json& json, RequestType request_type) {
        if (json.contains("error")) {
            const auto& err = json["error"];
            std::string reason;

            if (err.is_object()) {
                if (err.contains("message") && err["message"].is_string()) {
                    reason = err["message"].get<std::string>();
                } else {
                    reason = err.dump();
                }
            } else if (err.is_string()) {
                reason = err.get<std::string>();
            } else {
                reason = err.dump();
            }

            ThrowIfTokenLimitProviderError(reason);
            trigger_error("Provider error: " + reason);
            std::cerr << ">> response error :\n"
                      << json.dump(2) << "\n";
        }
        checkProviderSpecificResponse(json, request_type);
    }

    bool isJson(const std::string& data) {
        try {
            [[maybe_unused]] auto _ = nlohmann::json::parse(data);
        } catch (...) {
            return false;
        }
        return true;
    }
};

}// namespace flock
