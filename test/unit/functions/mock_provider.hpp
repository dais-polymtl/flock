#pragma once
#include "flockmtl/model_manager/providers/provider.hpp"
#include <gmock/gmock.h>

namespace flockmtl {

class MockProvider : public IProvider {
public:
    explicit MockProvider(const ModelDetails& model_details) : IProvider(model_details) {}

    MOCK_METHOD(void, AddCompletionRequest, (const std::string& prompt, const int num_output_tuples, bool json_response, OutputType output_type), (override));
    MOCK_METHOD(void, AddEmbeddingRequest, (const std::vector<std::string>& inputs), (override));
    MOCK_METHOD(std::vector<nlohmann::json>, CollectCompletions, (const std::string& contentType), (override));
    MOCK_METHOD(std::vector<nlohmann::json>, CollectEmbeddings, (const std::string& contentType), (override));
};

}// namespace flockmtl
