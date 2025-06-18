#pragma once

#include "flockmtl/model_manager/providers/adapters/openai.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>

namespace flockmtl {

// Mock class for OpenAI API to avoid real API calls during tests
class MockOpenAIProvider : public OpenAIProvider {
public:
    explicit MockOpenAIProvider() : OpenAIProvider(ModelDetails()) {}

    // Override the API call methods for testing
    MOCK_METHOD(nlohmann::json, CallComplete, (const std::string& prompt, bool json_response), (override));
    MOCK_METHOD(nlohmann::json, CallEmbedding, (const std::vector<std::string>& inputs), (override));
};

}// namespace flockmtl
