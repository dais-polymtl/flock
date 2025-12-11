#include "flock/model_manager/model.hpp"
#include "nlohmann/json.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace flock {
using json = nlohmann::json;

class ModelManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto con = Config::GetConnection();
        con.Query(" CREATE SECRET ("
                  "       TYPE OPENAI,"
                  "    API_KEY 'your-api-key');");
        con.Query("      CREATE SECRET ("
                  "            TYPE AZURE_LLM,"
                  "         API_KEY 'your-api-key',"
                  "   RESOURCE_NAME 'your-resource-name',"
                  "     API_VERSION '2023-05-15');");
        con.Query("  CREATE SECRET ("
                  "       TYPE OLLAMA,"
                  "    API_URL '127.0.0.1:11434');");
    }

    void TearDown() override {
    }
};

// Test Model initialization with valid model configuration
TEST_F(ModelManagerTest, ModelInitialization) {
    // Create a model configuration with required parameters
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 32},
            {"model_parameters", "{\"temperature\": 0.7}"}};

    EXPECT_NO_THROW({
        Model model(model_config);
        ModelDetails details = model.GetModelDetails();
        EXPECT_EQ(details.model_name, "gpt-4o-test");
        EXPECT_EQ(details.model, "gpt-4o");
        EXPECT_EQ(details.provider_name, "openai");
        EXPECT_EQ(details.model_parameters, nlohmann::json::parse("{\"temperature\": 0.7}"));
        EXPECT_EQ(details.tuple_format, "json");
        EXPECT_EQ(details.batch_size, 32);
    });
}

// Test Model initialization with minimal required parameters
TEST_F(ModelManagerTest, ModelInitializationMinimal) {
    // Create a model config with only model_name (other details should be fetched from DB)
    json model_config = {
            {"model_name", "gpt-4o-test"}};
    // Model initialization should fetch remaining details from database
    EXPECT_NO_THROW({
        Model model(model_config);
        ModelDetails details = model.GetModelDetails();
        EXPECT_EQ(details.model_name, "gpt-4o-test");
        EXPECT_EQ(details.model, "gpt-4o");
        EXPECT_EQ(details.provider_name, "openai");
    });
}

// Test Model initialization with invalid configuration
TEST_F(ModelManagerTest, ModelInitializationInvalid) {
    // Create a model config without required model_name
    json invalid_model_config = {
            {"model", "gpt-4o-test"},
            {"provider", "openai"}};
    // Model initialization should fail due to missing model_name
    EXPECT_THROW(Model model(invalid_model_config), std::invalid_argument);
    // Model name that doesn't exist in the system
    json nonexistent_model = {
            {"model_name", "non_existent_model_123"}};
    // Model initialization should fail for non-existent model
    EXPECT_THROW(Model model(nonexistent_model), std::runtime_error);
}

// Test model provider selection based on provider name
TEST_F(ModelManagerTest, ProviderSelection) {
    // Test OpenAI provider
    json openai_config = {
            {"model_name", "gpt-4o-test"}};
    EXPECT_NO_THROW({
        Model openai_model(openai_config);
        EXPECT_EQ(openai_model.GetModelDetails().provider_name, "openai");
    });
    // Test Azure provider
    json azure_config = {
            {"model_name", "azure-gpt-4o-mini"}};
    EXPECT_NO_THROW({
        Model azure_model(azure_config);
        EXPECT_EQ(azure_model.GetModelDetails().provider_name, "azure");
    });
    // Test Ollama provider
    json ollama_config = {
            {"model_name", "gemma3:4b"}};
    EXPECT_NO_THROW({
        Model ollama_model(ollama_config);
        EXPECT_EQ(ollama_model.GetModelDetails().provider_name, "ollama");
    });
}

// Test model details retrieval
TEST_F(ModelManagerTest, GetModelDetails) {
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"model_parameters", "{\"temperature\": 0.7}"},
            {"tuple_format", "XML"},
            {"batch_size", 10}};

    Model model(model_config);
    ModelDetails details = model.GetModelDetails();

    EXPECT_EQ(details.model_name, "gpt-4o-test");
    EXPECT_EQ(details.model, "gpt-4o");
    EXPECT_EQ(details.provider_name, "openai");
    EXPECT_EQ(details.model_parameters, nlohmann::json::parse("{\"temperature\": 0.7}"));
    EXPECT_EQ(details.tuple_format, "XML");
    EXPECT_EQ(details.batch_size, 10);
}

}// namespace flock