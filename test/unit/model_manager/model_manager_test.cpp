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
        EXPECT_EQ(details.tuple_format, TupleFormat::JSON);
        EXPECT_EQ(details.batch_size, 32);
        EXPECT_TRUE(details.is_async);
    });
}

TEST_F(ModelManagerTest, ModelInitializationParsesInlineIsAsync) {
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 32},
            {"model_parameters", nlohmann::json::object()},
            {"is_async", true}};

    EXPECT_NO_THROW({
        Model model(model_config);
        ModelDetails details = model.GetModelDetails();
        EXPECT_TRUE(details.is_async);
        EXPECT_EQ(model.GetModelDetailsAsJson()["is_async"], true);
    });
}

TEST_F(ModelManagerTest, ModelInitializationParsesInlineSyncMode) {
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 32},
            {"model_parameters", nlohmann::json::object()},
            {"is_async", false}};

    EXPECT_NO_THROW({
        Model model(model_config);
        ModelDetails details = model.GetModelDetails();
        EXPECT_FALSE(details.is_async);
        EXPECT_EQ(model.GetModelDetailsAsJson()["is_async"], false);
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
        EXPECT_EQ(details.batch_size, 32);
    });
}

TEST_F(ModelManagerTest, ModelInitializationUsesDefaultBatchSizeWhenUnset) {
    json model_config = {
            {"model_name", "gpt-4o"}};

    EXPECT_NO_THROW({
        Model model(model_config);
        ModelDetails details = model.GetModelDetails();
        EXPECT_EQ(details.model_name, "gpt-4o");
        EXPECT_EQ(details.model, "gpt-4o");
        EXPECT_EQ(details.provider_name, "openai");
        EXPECT_EQ(details.batch_size, DEFAULT_BATCH_SIZE);
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
    EXPECT_EQ(details.tuple_format, TupleFormat::XML);
    EXPECT_EQ(details.batch_size, 10);
}

TEST_F(ModelManagerTest, ModelInitializationRejectsNonPositiveBatchSize) {
    const json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 0}};

    EXPECT_THROW(Model model(model_config), std::runtime_error);
    EXPECT_THROW(Model({{"model_name", "gpt-4o-test"},
                        {"model", "gpt-4o"},
                        {"provider", "openai"},
                        {"tuple_format", "json"},
                        {"batch_size", -1}}),
                 std::runtime_error);
}

TEST_F(ModelManagerTest, ModelInitializationParsesRateLimit) {
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 32},
            {"model_parameters", nlohmann::json::object()},
            {"rate_limit", 30}};

    Model model(model_config);
    ModelDetails details = model.GetModelDetails();
    ASSERT_TRUE(details.rate_limit.has_value());
    EXPECT_EQ(details.rate_limit.value(), 30);
    EXPECT_EQ(model.GetModelDetailsAsJson()["rate_limit"], 30);
}

TEST_F(ModelManagerTest, ModelInitializationCapsBatchSizeWithRateLimit) {
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 64},
            {"model_parameters", nlohmann::json::object()},
            {"rate_limit", 10}};

    Model model(model_config);
    ModelDetails details = model.GetModelDetails();
    EXPECT_EQ(details.batch_size, 10);
}

TEST_F(ModelManagerTest, ModelInitializationWithoutRateLimit) {
    json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 32},
            {"model_parameters", nlohmann::json::object()}};

    Model model(model_config);
    ModelDetails details = model.GetModelDetails();
    EXPECT_FALSE(details.rate_limit.has_value());
    EXPECT_FALSE(model.GetModelDetailsAsJson().contains("rate_limit"));
}

TEST_F(ModelManagerTest, ModelInitializationRejectsNonPositiveRateLimit) {
    const json model_config = {
            {"model_name", "gpt-4o-test"},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", "json"},
            {"batch_size", 32},
            {"rate_limit", 0}};

    EXPECT_THROW(Model model(model_config), std::runtime_error);
}

}// namespace flock
