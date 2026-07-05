#pragma once

#include "flock/core/common.hpp"
#include "flock/prompt_manager/repository.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

namespace flock {

inline constexpr int DEFAULT_BATCH_SIZE = 16;

struct TotalUsage {
    size_t prompt_tokens = 0;
    size_t completion_tokens = 0;

    size_t total_tokens() const { return prompt_tokens + completion_tokens; }

    TotalUsage& operator+=(const TotalUsage& other) {
        prompt_tokens += other.prompt_tokens;
        completion_tokens += other.completion_tokens;
        return *this;
    }
};

struct UsageLimit {
    std::optional<size_t> prompt_tokens_limit;
    std::optional<size_t> completion_tokens_limit;
    std::optional<size_t> total_tokens_limit;

    bool HasAnyLimit() const {
        return prompt_tokens_limit.has_value() || completion_tokens_limit.has_value() ||
               total_tokens_limit.has_value();
    }
};

inline size_t ParsePositiveSizeFromJson(const nlohmann::json& value, const std::string& field_name) {
    const int parsed = value.get<int>();
    if (parsed <= 0) {
        throw std::runtime_error("'" + field_name + "' must be larger than 0");
    }
    return static_cast<size_t>(parsed);
}

inline UsageLimit ParseUsageLimitFromJson(const nlohmann::json& value) {
    UsageLimit limit;
    if (value.contains("prompt_tokens_limit")) {
        limit.prompt_tokens_limit = ParsePositiveSizeFromJson(value.at("prompt_tokens_limit"), "prompt_tokens_limit");
    }
    if (value.contains("completion_tokens_limit")) {
        limit.completion_tokens_limit =
                ParsePositiveSizeFromJson(value.at("completion_tokens_limit"), "completion_tokens_limit");
    }
    if (value.contains("total_tokens_limit")) {
        limit.total_tokens_limit = ParsePositiveSizeFromJson(value.at("total_tokens_limit"), "total_tokens_limit");
    }
    return limit;
}

inline nlohmann::json UsageLimitToJson(const UsageLimit& limit) {
    nlohmann::json result = nlohmann::json::object();
    if (limit.prompt_tokens_limit.has_value()) {
        result["prompt_tokens_limit"] = *limit.prompt_tokens_limit;
    }
    if (limit.completion_tokens_limit.has_value()) {
        result["completion_tokens_limit"] = *limit.completion_tokens_limit;
    }
    if (limit.total_tokens_limit.has_value()) {
        result["total_tokens_limit"] = *limit.total_tokens_limit;
    }
    return result;
}

struct ModelDetails {
    std::string provider_name;
    std::string model_name;
    std::string model;
    std::unordered_map<std::string, std::string> secret;
    TupleFormat tuple_format = TupleFormat::XML;
    size_t max_batch_size;
    nlohmann::json model_parameters;
    bool is_async = true;
    std::optional<size_t> rate_limit;
    std::optional<UsageLimit> usage_limit;
};


inline int ResolveMaxBatchSizeFromJson(const nlohmann::json& model_args) {
    const bool has_max = model_args.contains("max_batch_size");
    const bool has_legacy = model_args.contains("batch_size");

    if (has_max) {
        if (has_legacy) {
            std::cerr << "[Flock] Warning: 'batch_size' is getting deprecated; use 'max_batch_size' instead.\n";
        }
        return static_cast<int>(ParsePositiveSizeFromJson(model_args.at("max_batch_size"), "max_batch_size"));
    }

    if (has_legacy) {
        std::cerr << "[Flock] Warning: 'batch_size' is getting deprecated; use 'max_batch_size' instead.\n";
        return static_cast<int>(ParsePositiveSizeFromJson(model_args.at("batch_size"), "max_batch_size"));
    }

    return DEFAULT_BATCH_SIZE;
}


const std::string OLLAMA = "ollama";
const std::string OPENAI = "openai";
const std::string AZURE = "azure";
const std::string ANTHROPIC = "anthropic";
const std::string DEFAULT_PROVIDER = "default";
const std::string EMPTY_PROVIDER = "";

// Provider-specific API versions
const std::string ANTHROPIC_DEFAULT_API_VERSION = "2023-06-01";

enum SupportedProviders {
    FLOCKMTL_OPENAI = 0,
    FLOCKMTL_AZURE,
    FLOCKMTL_OLLAMA,
    FLOCKMTL_ANTHROPIC,
    FLOCKMTL_UNSUPPORTED_PROVIDER,
    FLOCKMTL_SUPPORTED_PROVIDER_COUNT
};

inline SupportedProviders GetProviderType(std::string provider) {
    std::transform(provider.begin(), provider.end(), provider.begin(), [](unsigned char c) { return std::tolower(c); });
    if (provider == OPENAI || provider == DEFAULT_PROVIDER || provider == EMPTY_PROVIDER)
        return FLOCKMTL_OPENAI;
    if (provider == AZURE)
        return FLOCKMTL_AZURE;
    if (provider == OLLAMA)
        return FLOCKMTL_OLLAMA;
    if (provider == ANTHROPIC)
        return FLOCKMTL_ANTHROPIC;

    return FLOCKMTL_UNSUPPORTED_PROVIDER;
}

inline std::string GetProviderName(SupportedProviders provider) {
    switch (provider) {
        case FLOCKMTL_OPENAI:
            return OPENAI;
        case FLOCKMTL_AZURE:
            return AZURE;
        case FLOCKMTL_OLLAMA:
            return OLLAMA;
        case FLOCKMTL_ANTHROPIC:
            return ANTHROPIC;
        default:
            return "";
    }
}

}// namespace flock
