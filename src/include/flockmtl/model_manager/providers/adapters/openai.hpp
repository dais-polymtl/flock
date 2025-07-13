#pragma once

#include "flockmtl/model_manager/providers/handlers/openai.hpp"
#include "flockmtl/model_manager/providers/provider.hpp"

namespace flockmtl {

class OpenAIProvider : public IProvider {
public:
    OpenAIProvider(const ModelDetails& model_details) : IProvider(model_details) {
        auto base_url = std::string("");
        if (const auto it = model_details_.secret.find("base_url"); it != model_details_.secret.end()) {
            base_url = it->second;
        }
        model_handler_ = std::make_unique<OpenAIModelManager>(
                model_details_.secret["api_key"], base_url, true);
    }

    nlohmann::json CallComplete(const std::string& prompt, bool json_response, OutputType output_type) override;
    nlohmann::json CallEmbedding(const std::vector<std::string>& inputs) override;
};

}// namespace flockmtl
