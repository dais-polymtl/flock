#pragma once

#include "flockmtl/model_manager/providers/handlers/azure.hpp"
#include "flockmtl/model_manager/providers/provider.hpp"

namespace flockmtl {

class AzureProvider : public IProvider {
public:
    AzureProvider(const ModelDetails& model_details) : IProvider(model_details) {
        model_handler_ =
                std::make_unique<AzureModelManager>(model_details_.secret["api_key"], model_details_.secret["resource_name"],
                                                    model_details_.model, model_details_.secret["api_version"], true);
    }

    nlohmann::json CallComplete(const std::string& prompt, bool json_response, OutputType output_type) override;
    nlohmann::json CallEmbedding(const std::vector<std::string>& inputs) override;
};

}// namespace flockmtl
