#pragma once

#include "flock/core/common.hpp"
#include <algorithm>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace flock {

inline constexpr int DEFAULT_BATCH_SIZE = 16;
inline constexpr int DEFAULT_MAX_ASYNC_CALLS = 20;

struct ModelDetails {
    std::string provider_name;
    std::string model_name;
    std::string model;
    std::unordered_map<std::string, std::string> secret;
    std::string tuple_format;
    int batch_size;
    int max_async_calls;
    nlohmann::json model_parameters;
};

const std::string OLLAMA = "ollama";
const std::string OPENAI = "openai";
const std::string AZURE = "azure";
const std::string ANTHROPIC = "anthropic";
const std::string DEMO = "demo";
const std::string DEFAULT_PROVIDER = "default";
const std::string EMPTY_PROVIDER = "";

// Provider-specific API versions
const std::string ANTHROPIC_DEFAULT_API_VERSION = "2023-06-01";

enum SupportedProviders {
    FLOCKMTL_OPENAI = 0,
    FLOCKMTL_AZURE,
    FLOCKMTL_OLLAMA,
    FLOCKMTL_ANTHROPIC,
    FLOCKMTL_DEMO,
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
    if (provider == DEMO)
        return FLOCKMTL_DEMO;

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
        case FLOCKMTL_DEMO:
            return DEMO;
        default:
            return "";
    }
}

}// namespace flock
