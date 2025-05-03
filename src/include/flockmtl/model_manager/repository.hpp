#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>

namespace flockmtl {

struct ModelDetails {
    std::string provider_name;
    std::string model_name;
    std::string model;
    int32_t context_window;
    int32_t max_output_tokens;
    float temperature;
    std::unordered_map<std::string, std::string> secret;
    std::string tuple_format;
    int batch_size;
};

const std::string OLLAMA = "ollama";
const std::string OPENAI = "openai";
const std::string AZURE = "azure";
const std::string DEFAULT_PROVIDER = "default";
const std::string EMPTY_PROVIDER = "";

enum SupportedProviders {
    FLOCKMTL_OPENAI = 0,
    FLOCKMTL_AZURE,
    FLOCKMTL_OLLAMA,
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
        default:
            return "";
    }
}

}// namespace flockmtl
