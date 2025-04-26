#include "flockmtl/secret_manager/secret_manager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace flockmtl;
using namespace testing;

// Test the SecretDetails functions
TEST(SecretDetailsTest, OpenAIDetails) {
    auto details = get_openai_secret_details();
    EXPECT_EQ(details.type, "openai");
    EXPECT_EQ(details.provider, "flockmtl");
    EXPECT_EQ(details.scope, "openai://");
    EXPECT_THAT(details.fields, UnorderedElementsAre("base_url", "api_key"));
    EXPECT_THAT(details.redact_fields, UnorderedElementsAre("api_key"));
    EXPECT_THAT(details.required_fields, UnorderedElementsAre("api_key"));
}

TEST(SecretDetailsTest, AzureDetails) {
    auto details = get_azure_secret_details();
    EXPECT_EQ(details.type, "azure_llm");
    EXPECT_EQ(details.provider, "flockmtl");
    EXPECT_EQ(details.scope, "azure_llm://");
    EXPECT_THAT(details.fields, UnorderedElementsAre("api_key", "resource_name", "api_version"));
    EXPECT_THAT(details.redact_fields, UnorderedElementsAre("api_key"));
    EXPECT_THAT(details.required_fields, UnorderedElementsAre("api_key", "resource_name", "api_version"));
}

TEST(SecretDetailsTest, OllamaDetails) {
    auto details = get_ollama_secret_details();
    EXPECT_EQ(details.type, "ollama");
    EXPECT_EQ(details.provider, "flockmtl");
    EXPECT_EQ(details.scope, "ollama://");
    EXPECT_THAT(details.fields, UnorderedElementsAre("api_url"));
    EXPECT_THAT(details.redact_fields, UnorderedElementsAre("api_url"));
    EXPECT_THAT(details.required_fields, UnorderedElementsAre("api_url"));
}

TEST(SecretDetailsTest, DetailsList) {
    auto details_list = get_secret_details_list();
    EXPECT_EQ(details_list.size(), 3);
    EXPECT_EQ(details_list[0].type, "openai");
    EXPECT_EQ(details_list[1].type, "azure_llm");
    EXPECT_EQ(details_list[2].type, "ollama");
}