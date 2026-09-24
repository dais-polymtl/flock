#pragma once

#include "flock/model_manager/providers/handlers/base_handler.hpp"
#include "session.hpp"
#include <stdexcept>
#include <string>

namespace flock {

// TypeSafe's System One API, served by TypeSafe and by OpenRouter.
class TypeSafeModelManager : public BaseModelProviderHandler {
public:
    TypeSafeModelManager(std::string token, const std::string& api_base_url, bool throw_exception,
                         const std::string& model_name = "", std::optional<int> rate_limit = std::nullopt,
                         std::optional<UsageLimit> usage_limit = std::nullopt,
                         std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                         std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : BaseModelProviderHandler(throw_exception, model_name, rate_limit, std::move(usage_limit),
                                   std::move(rate_limiter), std::move(usage_limiter)),
          _token(std::move(token)), _session("TypeSafe", throw_exception) {
        _api_base_url = NormalizeBaseUrl(api_base_url.empty() ? std::string("https://api.typesafe.ai") : api_base_url);
        _session.setUrl(_api_base_url);
    }

    TypeSafeModelManager(const TypeSafeModelManager&) = delete;
    TypeSafeModelManager& operator=(const TypeSafeModelManager&) = delete;
    TypeSafeModelManager(TypeSafeModelManager&&) = delete;
    TypeSafeModelManager& operator=(TypeSafeModelManager&&) = delete;

private:
    // Accepts the host root, the '/v1' base or the full endpoint.
    static std::string NormalizeBaseUrl(std::string url) {
        const auto& strip_suffix = [&url](const std::string& suffix) {
            if (url.size() >= suffix.size() && url.compare(url.size() - suffix.size(), suffix.size(), suffix) == 0) {
                url.erase(url.size() - suffix.size());
            }
        };
        const auto strip_trailing_slashes = [&url]() {
            while (!url.empty() && url.back() == '/') {
                url.pop_back();
            }
        };
        strip_trailing_slashes();
        strip_suffix("/systemone");
        strip_suffix("/v1");
        strip_trailing_slashes();
        return url;
    }

protected:
    std::string _token;
    std::string _api_base_url;
    Session _session;

    std::string getCompletionUrl() const override {
        return _api_base_url + "/v1/systemone";
    }
    std::string getEmbedUrl() const override {
        throw std::runtime_error("[ModelProvider] TypeSafe does not provide an embeddings endpoint");
    }
    std::string getTranscriptionUrl() const override {
        throw std::runtime_error("[ModelProvider] TypeSafe does not provide a transcription endpoint");
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
        if (request_type != RequestType::Completion) {
            return;
        }

        if (response.contains("detail")) {
            const auto& detail = response["detail"];
            if (detail.is_object() && detail.value("error_type", "") == "max_tokens_exceeded") {
                throw TokenLimitExceededError("input_token", 0, 0);
            }
            throw std::runtime_error("TypeSafe rejected the request: " + detail.dump());
        }
        if (!response.contains("answers") || !response["answers"].is_object()) {
            throw std::runtime_error("TypeSafe response contained no answers object");
        }
    }

    // Returned as-is: the provider maps answers back onto rows.
    nlohmann::json ExtractCompletionOutput(const nlohmann::json& response) const override {
        return response;
    }

    nlohmann::json ExtractTranscriptionOutput(const nlohmann::json&) const override {
        throw std::runtime_error("[ModelProvider] TypeSafe does not support transcription");
    }

    std::pair<int64_t, int64_t> ExtractTokenUsage(const nlohmann::json& response) const override {
        int64_t input_tokens = 0;
        int64_t output_tokens = 0;
        if (response.contains("usage") && response["usage"].is_object()) {
            const auto& usage = response["usage"];
            if (usage.contains("input_tokens") && usage["input_tokens"].is_number()) {
                input_tokens = usage["input_tokens"].get<int64_t>();
            }
            // Free on TypeSafe, but still reported.
            if (usage.contains("output_tokens") && usage["output_tokens"].is_number()) {
                output_tokens = usage["output_tokens"].get<int64_t>();
            }
        }
        return {input_tokens, output_tokens};
    }
};

}// namespace flock
