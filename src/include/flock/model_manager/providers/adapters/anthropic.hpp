#pragma once

#include "flock/model_manager/providers/handlers/anthropic.hpp"
#include "flock/model_manager/providers/provider.hpp"

namespace flock {

class AnthropicProvider : public IProvider {
public:
    AnthropicProvider(const ModelDetails& model_details) : IProvider(model_details) {
        auto api_version = ANTHROPIC_DEFAULT_API_VERSION;
        if (const auto it = model_details_.secret.find("api_version");
            it != model_details_.secret.end()) {
            api_version = it->second;
        }
        model_handler_ = std::make_unique<AnthropicModelManager>(
                model_details_.secret.at("api_key"), api_version, true);
    }

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples,
                              OutputType output_type, const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
};

}// namespace flock
