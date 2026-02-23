#include "duckdb/main/secret/secret_manager.hpp"
#include "flock/core/config.hpp"
#include "flock/secret_manager/secret_manager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace flock {

// Test the SecretDetails functions
TEST(SecretManager, OpenAIDetails) {
    auto details = get_openai_secret_details();
    EXPECT_EQ(details.type, "openai");
    EXPECT_EQ(details.provider, "flock");
    EXPECT_EQ(details.scope, "openai://");
    EXPECT_THAT(details.fields, testing::UnorderedElementsAre("base_url", "api_key"));
    EXPECT_THAT(details.redact_fields, testing::UnorderedElementsAre("api_key"));
    EXPECT_THAT(details.required_fields, testing::UnorderedElementsAre("api_key"));
}

TEST(SecretManager, AzureDetails) {
    auto details = get_azure_secret_details();
    EXPECT_EQ(details.type, "azure_llm");
    EXPECT_EQ(details.provider, "flock");
    EXPECT_EQ(details.scope, "azure_llm://");
    EXPECT_THAT(details.fields, testing::UnorderedElementsAre("api_key", "resource_name", "api_version"));
    EXPECT_THAT(details.redact_fields, testing::UnorderedElementsAre("api_key"));
    EXPECT_THAT(details.required_fields, testing::UnorderedElementsAre("api_key", "resource_name", "api_version"));
}

TEST(SecretManager, OllamaDetails) {
    auto details = get_ollama_secret_details();
    EXPECT_EQ(details.type, "ollama");
    EXPECT_EQ(details.provider, "flock");
    EXPECT_EQ(details.scope, "ollama://");
    EXPECT_THAT(details.fields, testing::UnorderedElementsAre("api_url"));
    EXPECT_THAT(details.redact_fields, testing::UnorderedElementsAre("api_url"));
    EXPECT_THAT(details.required_fields, testing::UnorderedElementsAre("api_url"));
}

TEST(SecretManager, DetailsList) {
    auto details_list = get_secret_details_list();
    EXPECT_EQ(details_list.size(), 4);
    EXPECT_EQ(details_list[0].type, "openai");
    EXPECT_EQ(details_list[1].type, "azure_llm");
    EXPECT_EQ(details_list[2].type, "ollama");
    EXPECT_EQ(details_list[3].type, "anthropic");
}

// Test the provider-related functions
TEST(SecretManager, GetProviderType) {
    // Test that each provider string maps to the correct enum value
    EXPECT_EQ(SecretManager::GetProviderType("openai"), SecretManager::SupportedProviders::OPENAI);
    EXPECT_EQ(SecretManager::GetProviderType("azure_llm"), SecretManager::SupportedProviders::AZURE);
    EXPECT_EQ(SecretManager::GetProviderType("ollama"), SecretManager::SupportedProviders::OLLAMA);

    // Test that an invalid provider string throws an exception
    EXPECT_THROW({ SecretManager::GetProviderType("invalid_provider"); }, std::runtime_error);
}

TEST(SecretManager, ValidateRequiredFields) {
    // Mock input with all required fields present
    duckdb::CreateSecretInput valid_input;
    valid_input.options["api_key"] = duckdb::Value("test-key");
    valid_input.options["resource_name"] = duckdb::Value("test-resource");
    valid_input.options["api_version"] = duckdb::Value("2023-05-15");

    // Test with valid input and all required fields
    std::vector<std::string> required_fields = {"api_key", "resource_name", "api_version"};
    EXPECT_NO_THROW(SecretManager::ValidateRequiredFields(valid_input, required_fields));

    // Test with missing required field
    duckdb::CreateSecretInput invalid_input;
    invalid_input.options["api_key"] = duckdb::Value("test-key");
    // Missing resource_name and api_version

    EXPECT_THROW({ SecretManager::ValidateRequiredFields(invalid_input, required_fields); }, std::runtime_error);

    // Test with empty required field value
    duckdb::CreateSecretInput empty_value_input;
    empty_value_input.options["api_key"] = duckdb::Value("test-key");
    empty_value_input.options["resource_name"] = duckdb::Value("");// Empty value
    empty_value_input.options["api_version"] = duckdb::Value("2023-05-15");

    EXPECT_THROW({ SecretManager::ValidateRequiredFields(empty_value_input, required_fields); }, std::runtime_error);
}

TEST(SecretManager, GetOpenAISecret) {
    auto con = Config::GetConnection();

    con.Query(" CREATE SECRET ("
              "       TYPE OPENAI,"
              "    API_KEY 'your-api-key');");

    auto result = SecretManager::GetSecret("__default_openai");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result["api_key"], "your-api-key");
}

TEST(SecretManager, GetOpenAICompatibleSecret) {
    auto con = Config::GetConnection();

    con.Query(" CREATE SECRET openai_compatible ("
              "       TYPE OPENAI,"
              "    API_KEY 'your-api-key',"
              "    BASE_URL 'https://api.provider.com/v1');");

    auto result = SecretManager::GetSecret("openai_compatible");
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["api_key"], "your-api-key");
    EXPECT_EQ(result["base_url"], "https://api.provider.com/v1");
}

TEST(SecretManager, GetAzureSecret) {
    auto con = Config::GetConnection();

    con.Query("      CREATE SECRET ("
              "            TYPE AZURE_LLM,"
              "         API_KEY 'your-api-key',"
              "   RESOURCE_NAME 'your-resource-name',"
              "     API_VERSION '2023-05-15');");

    auto result = SecretManager::GetSecret("__default_azure_llm");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result["api_key"], "your-api-key");
    EXPECT_EQ(result["resource_name"], "your-resource-name");
    EXPECT_EQ(result["api_version"], "2023-05-15");
}

TEST(SecretManager, GetOllamaSecret) {
    auto con = Config::GetConnection();

    con.Query("  CREATE SECRET ("
              "TYPE OLLAMA,"
              "API_URL '127.0.0.1:11434');");

    auto result = SecretManager::GetSecret("__default_ollama");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result["api_url"], "127.0.0.1:11434");
}

}// namespace flock