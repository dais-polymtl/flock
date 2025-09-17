#include "../functions/mock_provider.hpp"
#include "flock/model_manager/providers/adapters/azure.hpp"
#include "flock/model_manager/providers/adapters/ollama.hpp"
#include "flock/model_manager/providers/adapters/openai.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace flock {
using json = nlohmann::json;

// Test OpenAI provider behavior
TEST(ModelProvidersTest, OpenAIProviderTest) {
    ModelDetails model_details;
    model_details.model_name = "test_model";
    model_details.model = "gpt-4";
    model_details.provider_name = "openai";
    model_details.model_parameters = {{"temperature", 0.7}};
    model_details.secret = {{"api_key", "test_api_key"}};

    // Create a mock provider
    MockProvider mock_provider(model_details);

    // Set up mock behavior for AddCompletionRequest and CollectCompletions
    const std::string test_prompt = "Test prompt for completion";
    const json expected_complete_response = {{"response", "This is a test response"}};

    EXPECT_CALL(mock_provider, AddCompletionRequest(test_prompt, 1, OutputType::STRING, nlohmann::json::array()))
            .Times(1);
    EXPECT_CALL(mock_provider, CollectCompletions("application/json"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_complete_response}));

    // Set up mock behavior for AddEmbeddingRequest and CollectEmbeddings
    const std::vector<std::string> test_inputs = {"Test input for embedding"};
    const json expected_embedding_response = json::array({{0.1, 0.2, 0.3, 0.4, 0.5}});

    EXPECT_CALL(mock_provider, AddEmbeddingRequest(test_inputs))
            .Times(1);
    EXPECT_CALL(mock_provider, CollectEmbeddings("application/json"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_embedding_response}));

    // Test the mocked methods
    mock_provider.AddCompletionRequest(test_prompt, 1, OutputType::STRING, nlohmann::json::array());
    auto complete_results = mock_provider.CollectCompletions("application/json");
    ASSERT_EQ(complete_results.size(), 1);
    EXPECT_EQ(complete_results[0], expected_complete_response);

    mock_provider.AddEmbeddingRequest(test_inputs);
    auto embedding_results = mock_provider.CollectEmbeddings("application/json");
    ASSERT_EQ(embedding_results.size(), 1);
    EXPECT_EQ(embedding_results[0], expected_embedding_response);
}

// Test Azure provider behavior
TEST(ModelProvidersTest, AzureProviderTest) {
    ModelDetails model_details;
    model_details.model_name = "test_model";
    model_details.model = "gpt-4";
    model_details.provider_name = "azure";
    model_details.model_parameters = {{"temperature", 0.7}};
    model_details.secret = {
            {"api_key", "test_api_key"},
            {"resource_name", "test_resource"},
            {"api_version", "2023-05-15"}};

    // Create a mock provider
    MockProvider mock_provider(model_details);

    // Set up mock behavior for AddCompletionRequest and CollectCompletions
    const std::string test_prompt = "Test prompt for completion";
    const json expected_complete_response = {{"response", "This is a test response from Azure"}};

    EXPECT_CALL(mock_provider, AddCompletionRequest(test_prompt, 1, OutputType::STRING, nlohmann::json::array()))
            .Times(1);
    EXPECT_CALL(mock_provider, CollectCompletions("application/json"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_complete_response}));

    // Set up mock behavior for AddEmbeddingRequest and CollectEmbeddings
    const std::vector<std::string> test_inputs = {"Test input for embedding"};
    const json expected_embedding_response = json::array({{0.5, 0.4, 0.3, 0.2, 0.1}});

    EXPECT_CALL(mock_provider, AddEmbeddingRequest(test_inputs))
            .Times(1);
    EXPECT_CALL(mock_provider, CollectEmbeddings("application/json"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_embedding_response}));

    // Test the mocked methods
    mock_provider.AddCompletionRequest(test_prompt, 1, OutputType::STRING, nlohmann::json::array());
    auto complete_results = mock_provider.CollectCompletions("application/json");
    ASSERT_EQ(complete_results.size(), 1);
    EXPECT_EQ(complete_results[0], expected_complete_response);

    mock_provider.AddEmbeddingRequest(test_inputs);
    auto embedding_results = mock_provider.CollectEmbeddings("application/json");
    ASSERT_EQ(embedding_results.size(), 1);
    EXPECT_EQ(embedding_results[0], expected_embedding_response);
}

// Test Ollama provider behavior
TEST(ModelProvidersTest, OllamaProviderTest) {
    ModelDetails model_details;
    model_details.model_name = "test_model";
    model_details.model = "llama3";
    model_details.provider_name = "ollama";
    model_details.model_parameters = {{"temperature", 0.7}};
    model_details.secret = {{"api_url", "http://localhost:11434"}};

    // Create a mock provider
    MockProvider mock_provider(model_details);

    // Set up mock behavior for AddCompletionRequest and CollectCompletions
    const std::string test_prompt = "Test prompt for Ollama completion";
    const json expected_complete_response = {{"response", "This is a test response from Ollama"}};

    EXPECT_CALL(mock_provider, AddCompletionRequest(test_prompt, 1, OutputType::STRING, nlohmann::json::array()))
            .Times(1);
    EXPECT_CALL(mock_provider, CollectCompletions("application/json"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_complete_response}));

    // Set up mock behavior for AddEmbeddingRequest and CollectEmbeddings
    const std::vector<std::string> test_inputs = {"Test input for Ollama embedding"};
    const json expected_embedding_response = json::array({{0.7, 0.6, 0.5, 0.4, 0.3}});

    EXPECT_CALL(mock_provider, AddEmbeddingRequest(test_inputs))
            .Times(1);
    EXPECT_CALL(mock_provider, CollectEmbeddings("application/json"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_embedding_response}));

    // Test the mocked methods
    mock_provider.AddCompletionRequest(test_prompt, 1, OutputType::STRING, nlohmann::json::array());
    auto complete_results = mock_provider.CollectCompletions("application/json");
    ASSERT_EQ(complete_results.size(), 1);
    EXPECT_EQ(complete_results[0], expected_complete_response);

    mock_provider.AddEmbeddingRequest(test_inputs);
    auto embedding_results = mock_provider.CollectEmbeddings("application/json");
    ASSERT_EQ(embedding_results.size(), 1);
    EXPECT_EQ(embedding_results[0], expected_embedding_response);
}

}// namespace flock