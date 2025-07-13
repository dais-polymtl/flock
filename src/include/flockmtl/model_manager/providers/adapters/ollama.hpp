#pragma once

#include "flockmtl/model_manager/providers/handlers/ollama.hpp"
#include "flockmtl/model_manager/providers/provider.hpp"

namespace flockmtl {

class OllamaProvider : public IProvider {
public:
    OllamaProvider(const ModelDetails& model_details) : IProvider(model_details) {
        model_handler_ = std::make_unique<OllamaModelManager>(model_details_.secret["api_url"], true);
    }

    nlohmann::json CallComplete(const std::string& prompt, bool json_response, OutputType output_type) override;
    nlohmann::json CallEmbedding(const std::vector<std::string>& inputs) override;
};

}// namespace flockmtl
