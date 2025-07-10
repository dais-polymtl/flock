#pragma once

#include "flockmtl/model_manager/providers/handlers/openai.hpp"
#include "flockmtl/model_manager/providers/provider.hpp"

namespace flockmtl {

class OpenAIProvider : public IProvider {
public:
    OpenAIProvider(const ModelDetails& model_details) : IProvider(model_details) {}

    nlohmann::json CallComplete(const std::string& prompt, bool json_response, OutputType output_type) override;
    nlohmann::json CallEmbedding(const std::vector<std::string>& inputs) override;
};

}// namespace flockmtl
