#include "flockmtl/secret_manager/secret_manager.hpp"
#include <duckdb/main/secret/secret_manager.hpp>
#include <gtest/gtest.h>

using namespace flockmtl;

TEST(SecretManagerValidationTest, ValidateRequiredFields) {
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