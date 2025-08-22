#pragma once

#include "flockmtl/model_manager/providers/handlers/ollama.hpp"
#include "flockmtl/model_manager/providers/provider.hpp"

namespace flockmtl {

class OllamaProvider : public IProvider {
public:
    OllamaProvider(const ModelDetails& model_details) : IProvider(model_details) {
        model_handler_ = std::make_unique<OllamaModelManager>(model_details_.secret["api_url"], true);
    }

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
};

}// namespace flockmtl
