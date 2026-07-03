#pragma once

#include "flock/model_manager/providers/handlers/openai.hpp"
#include "flock/model_manager/providers/provider.hpp"

namespace flock {

class OpenAIProvider : public IProvider {
public:
    OpenAIProvider(const ModelDetails& model_details, ModelRateLimiter* rate_limiter = nullptr,
                   ModelUsageLimiter* usage_limiter = nullptr)
        : IProvider(model_details, rate_limiter, usage_limiter) {
        auto base_url = std::string("");
        if (const auto it = model_details_.secret.find("base_url"); it != model_details_.secret.end()) {
            base_url = it->second;
        }
        model_handler_ = std::make_unique<OpenAIModelManager>(
                model_details_.secret["api_key"], base_url, true, model_details_.model_name,
                model_details_.rate_limit, model_details_.usage_limit, rate_limiter_, usage_limiter_);
    }

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
    void AddTranscriptionRequest(const nlohmann::json& audio_files) override;
};

}// namespace flock
