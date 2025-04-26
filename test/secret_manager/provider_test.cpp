#include "flockmtl/secret_manager/secret_manager.hpp"
#include <gtest/gtest.h>

using namespace flockmtl;

// Test the provider-related functions
TEST(SecretManagerProviderTest, GetProviderType) {
    // Test that each provider string maps to the correct enum value
    EXPECT_EQ(SecretManager::GetProviderType("openai"), SecretManager::SupportedProviders::OPENAI);
    EXPECT_EQ(SecretManager::GetProviderType("azure_llm"), SecretManager::SupportedProviders::AZURE);
    EXPECT_EQ(SecretManager::GetProviderType("ollama"), SecretManager::SupportedProviders::OLLAMA);

    // Test that an invalid provider string throws an exception
    EXPECT_THROW({ SecretManager::GetProviderType("invalid_provider"); }, std::runtime_error);
}
