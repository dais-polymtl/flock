#pragma once

#include "flock/model_manager/providers/handlers/anthropic.hpp"
#include "flock/model_manager/providers/provider.hpp"

namespace flock {

class AnthropicProvider : public IProvider {
public:
    AnthropicProvider(const ModelDetails& model_details, std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                      std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : IProvider(model_details, std::move(rate_limiter), std::move(usage_limiter)) {
        auto api_version = ANTHROPIC_DEFAULT_API_VERSION;
        if (const auto it = model_details_.secret.find("api_version");
            it != model_details_.secret.end()) {
            api_version = it->second;
        }
        std::string api_key = "";
        if (model_details_.secret.count("api_key")) {
            api_key = model_details_.secret.at("api_key");
        }
        std::string api_url = "";
        if (model_details_.secret.count("api_url")) {
            api_url = model_details_.secret.at("api_url");
            // Strip trailing path segments that will be added by getCompletionUrl
            // e.g., "http://localhost:11434/v1/messages" → "http://localhost:11434/v1/"
            if (api_url.size() > 10 && api_url.substr(api_url.size() - 8) == "/messages") {
                api_url = api_url.substr(0, api_url.size() - 8);
            }
        }
        model_handler_ = std::make_unique<AnthropicModelManager>(
                api_key, api_version, true, model_details_.model_name,
                model_details_.rate_limit, model_details_.usage_limit, rate_limiter_, usage_limiter_,
                api_url);
    }

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples,
                              OutputType output_type, const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
    void AddTranscriptionRequest(const nlohmann::json& audio_files) override;
};

}// namespace flock
