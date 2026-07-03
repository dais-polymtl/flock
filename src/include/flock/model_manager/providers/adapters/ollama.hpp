#pragma once

#include "flock/model_manager/providers/handlers/ollama.hpp"
#include "flock/model_manager/providers/provider.hpp"

namespace flock {

class OllamaProvider : public IProvider {
public:
    OllamaProvider(const ModelDetails& model_details, std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                   std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : IProvider(model_details, std::move(rate_limiter), std::move(usage_limiter)) {
        model_handler_ = std::make_unique<OllamaModelManager>(model_details_.secret["api_url"], true,
                                                              model_details_.model_name,
                                                              model_details_.rate_limit, model_details_.usage_limit,
                                                              rate_limiter_, usage_limiter_);
    }

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
    void AddTranscriptionRequest(const nlohmann::json& audio_files) override;
};

}// namespace flock
