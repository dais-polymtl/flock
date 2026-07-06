#pragma once

#include "duckdb/common/exception/http_exception.hpp"
#include "flock/core/common.hpp"
#include "flock/model_manager/providers/handlers/handler.hpp"
#include "flock/model_manager/repository.hpp"
#include <cctype>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>

namespace flock {

class ModelRateLimiter;
class ModelUsageLimiter;

bool is_base64(const std::string& str);

enum class OutputType {
    STRING,
    OBJECT,
    BOOL,
    INTEGER
};

class IProvider {
public:
    ModelDetails model_details_;
    std::unique_ptr<IModelProviderHandler> model_handler_;

    // Shared, app-lifetime limiters injected by the owner (see Model). They are
    // not owned here; the same instances are reused across every provider so
    // per-model rate/usage accounting is consistent process-wide.
    std::shared_ptr<ModelRateLimiter> rate_limiter_ = nullptr;
    std::shared_ptr<ModelUsageLimiter> usage_limiter_ = nullptr;

    explicit IProvider(const ModelDetails& model_details, std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                       std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : model_details_(model_details), rate_limiter_(std::move(rate_limiter)),
          usage_limiter_(std::move(usage_limiter)){};
    virtual ~IProvider() = default;

    virtual void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) = 0;
    virtual void AddEmbeddingRequest(const std::vector<std::string>& inputs) = 0;
    virtual void AddTranscriptionRequest(const nlohmann::json& audio_files) = 0;

    virtual std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") {
        return model_handler_->CollectCompletions(contentType);
    }
    virtual std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") {
        return model_handler_->CollectEmbeddings(contentType);
    }
    virtual std::vector<nlohmann::json> CollectTranscriptions(const std::string& contentType = "multipart/form-data") {
        return model_handler_->CollectTranscriptions(contentType);
    }

    static std::string GetOutputTypeString(const OutputType output_type) {
        switch (output_type) {
            case OutputType::STRING:
                return "string";
            case OutputType::OBJECT:
                return "object";
            case OutputType::BOOL:
                return "boolean";
            case OutputType::INTEGER:
                return "integer";
            default:
                throw std::invalid_argument("Unsupported output type");
        }
    }
};

class TokenLimitExceededError : public duckdb::HTTPException {
public:
    TokenLimitExceededError(const std::string& token_type = "output_token", const int64_t token_count = 0, const int64_t token_limit = 0)
        : duckdb::HTTPException(429, "",
                                duckdb::unordered_map<duckdb::string, duckdb::string>{},
                                "token_limit_exceeded",
                                "%s usage %lld exceeded limit of %lld for this model; increase max_output_tokens or check your model configuration.",
                                token_type, token_count, token_limit) {
    }
};

class UsageLimitExceededError : public duckdb::HTTPException {
public:
    UsageLimitExceededError(const std::string& token_type, const int64_t token_count, const int64_t token_limit)
        : duckdb::HTTPException(429, "",
                                duckdb::unordered_map<duckdb::string, duckdb::string>{},
                                "usage_limit_exceeded",
                                "%s usage %lld exceeded limit of %lld for this model; increase usage_limit or "
                                "reset usage.",
                                token_type, token_count, token_limit) {
    }
};


inline constexpr const char* TOKEN_LIMIT_EXCEEDED_KEY = "_flock_token_limit_exceeded";

inline nlohmann::json TokenLimitExceededMarker() {
    return nlohmann::json{{TOKEN_LIMIT_EXCEEDED_KEY, true}};
}

inline bool IsTokenLimitExceededMarker(const nlohmann::json& response) {
    return response.is_object() && response.contains(TOKEN_LIMIT_EXCEEDED_KEY) &&
           response[TOKEN_LIMIT_EXCEEDED_KEY].get<bool>();
}

inline std::string ToLowerAscii(std::string value) {
    for (auto& character: value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

inline bool IsTokenLimitErrorMessage(const std::string& reason) {
    const auto lower = ToLowerAscii(reason);
    static constexpr const char* patterns[] = {
            "context length",
            "context window",
            "maximum context",
            "max context",
            "token limit",
            "tokens exceed",
            "exceeds token",
            "too many tokens",
            "too long",
            "prompt is too long",
            "input is too long",
            "request too large",
            "max_tokens",
            "maximum tokens",
    };
    for (const auto* pattern: patterns) {
        if (lower.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

inline void ThrowIfTokenLimitProviderError(const std::string& reason) {
    if (IsTokenLimitErrorMessage(reason)) {
        throw TokenLimitExceededError();
    }
}

}// namespace flock
