#include "flockmtl/model_manager/providers/adapters/azure.hpp"
#include "flockmtl/model_manager/providers/adapters/ollama.hpp"
#include "flockmtl/model_manager/providers/adapters/openai.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace flockmtl {
using json = nlohmann::json;

// Mock class for OpenAI API to avoid real API calls during tests
class MockOpenAIProvider : public OpenAIProvider {
public:
    explicit MockOpenAIProvider(const ModelDetails& model_details) : OpenAIProvider(model_details) {}

    // Override the API call methods for testing
    MOCK_METHOD(nlohmann::json, CallComplete, (const std::string& prompt, bool json_response), (override));
    MOCK_METHOD(nlohmann::json, CallEmbedding, (const std::vector<std::string>& inputs), (override));
};

// Mock class for Azure API to avoid real API calls during tests
class MockAzureProvider : public AzureProvider {
public:
    explicit MockAzureProvider(const ModelDetails& model_details) : AzureProvider(model_details) {}

    // Override the API call methods for testing
    MOCK_METHOD(nlohmann::json, CallComplete, (const std::string& prompt, bool json_response), (override));
    MOCK_METHOD(nlohmann::json, CallEmbedding, (const std::vector<std::string>& inputs), (override));
};

// Mock class for Ollama API to avoid real API calls during tests
class MockOllamaProvider : public OllamaProvider {
public:
    explicit MockOllamaProvider(const ModelDetails& model_details) : OllamaProvider(model_details) {}

    // Override the API call methods for testing
    MOCK_METHOD(nlohmann::json, CallComplete, (const std::string& prompt, bool json_response), (override));
    MOCK_METHOD(nlohmann::json, CallEmbedding, (const std::vector<std::string>& inputs), (override));
};

// Test OpenAI provider behavior
TEST(ModelProvidersTest, OpenAIProviderTest) {
    ModelDetails model_details;
    model_details.model_name = "test_model";
    model_details.model = "gpt-4";
    model_details.provider_name = "openai";
    model_details.context_window = 128000;
    model_details.max_output_tokens = 8000;
    model_details.temperature = 0.7;
    model_details.secret = {{"api_key", "test_api_key"}};

    // Create a mock provider
    MockOpenAIProvider mock_provider(model_details);

    // Set up mock behavior for CallComplete
    const std::string test_prompt = "Test prompt for completion";
    const json expected_complete_response = {{"response", "This is a test response"}};

    EXPECT_CALL(mock_provider, CallComplete(test_prompt, true))
            .WillOnce(::testing::Return(expected_complete_response));

    // Set up mock behavior for CallEmbedding
    const std::vector<std::string> test_inputs = {"Test input for embedding"};
    const json expected_embedding_response = json::array({{0.1, 0.2, 0.3, 0.4, 0.5}});

    EXPECT_CALL(mock_provider, CallEmbedding(test_inputs))
            .WillOnce(::testing::Return(expected_embedding_response));

    // Test the mocked methods
    auto complete_result = mock_provider.CallComplete(test_prompt, true);
    EXPECT_EQ(complete_result, expected_complete_response);

    auto embedding_result = mock_provider.CallEmbedding(test_inputs);
    EXPECT_EQ(embedding_result, expected_embedding_response);
}

// Test Azure provider behavior
TEST(ModelProvidersTest, AzureProviderTest) {
    ModelDetails model_details;
    model_details.model_name = "test_model";
    model_details.model = "gpt-4";
    model_details.provider_name = "azure";
    model_details.context_window = 128000;
    model_details.max_output_tokens = 8000;
    model_details.temperature = 0.7;
    model_details.secret = {
            {"api_key", "test_api_key"},
            {"resource_name", "test_resource"},
            {"api_version", "2023-05-15"}};

    // Create a mock provider
    MockAzureProvider mock_provider(model_details);

    // Set up mock behavior for CallComplete
    const std::string test_prompt = "Test prompt for completion";
    const json expected_complete_response = {{"response", "This is a test response from Azure"}};

    EXPECT_CALL(mock_provider, CallComplete(test_prompt, true))
            .WillOnce(::testing::Return(expected_complete_response));

    // Set up mock behavior for CallEmbedding
    const std::vector<std::string> test_inputs = {"Test input for embedding"};
    const json expected_embedding_response = json::array({{0.5, 0.4, 0.3, 0.2, 0.1}});

    EXPECT_CALL(mock_provider, CallEmbedding(test_inputs))
            .WillOnce(::testing::Return(expected_embedding_response));

    // Test the mocked methods
    auto complete_result = mock_provider.CallComplete(test_prompt, true);
    EXPECT_EQ(complete_result, expected_complete_response);

    auto embedding_result = mock_provider.CallEmbedding(test_inputs);
    EXPECT_EQ(embedding_result, expected_embedding_response);
}

// Test Ollama provider behavior
TEST(ModelProvidersTest, OllamaProviderTest) {
    ModelDetails model_details;
    model_details.model_name = "test_model";
    model_details.model = "llama3";
    model_details.provider_name = "ollama";
    model_details.context_window = 128000;
    model_details.max_output_tokens = 8000;
    model_details.temperature = 0.7;
    model_details.secret = {{"api_url", "http://localhost:11434"}};

    // Create a mock provider
    MockOllamaProvider mock_provider(model_details);

    // Set up mock behavior for CallComplete
    const std::string test_prompt = "Test prompt for Ollama completion";
    const json expected_complete_response = {{"response", "This is a test response from Ollama"}};

    EXPECT_CALL(mock_provider, CallComplete(test_prompt, true))
            .WillOnce(::testing::Return(expected_complete_response));

    // Set up mock behavior for CallEmbedding
    const std::vector<std::string> test_inputs = {"Test input for Ollama embedding"};
    const json expected_embedding_response = json::array({{0.7, 0.6, 0.5, 0.4, 0.3}});

    EXPECT_CALL(mock_provider, CallEmbedding(test_inputs))
            .WillOnce(::testing::Return(expected_embedding_response));

    // Test the mocked methods
    auto complete_result = mock_provider.CallComplete(test_prompt, true);
    EXPECT_EQ(complete_result, expected_complete_response);

    auto embedding_result = mock_provider.CallEmbedding(test_inputs);
    EXPECT_EQ(embedding_result, expected_embedding_response);
}

}// namespace flockmtl