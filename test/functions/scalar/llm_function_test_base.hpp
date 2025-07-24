#pragma once

#include "../mock_provider.hpp"
#include "flockmtl/core/config.hpp"
#include "flockmtl/model_manager/model.hpp"
#include "flockmtl/model_manager/providers/provider.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace flockmtl {

// Base template class for LLM function tests
template<typename FunctionClass>
class LLMFunctionTestBase : public ::testing::Test {
protected:
    // Common test constants
    static constexpr const char* DEFAULT_MODEL = "gpt-4o";
    static constexpr const char* TEST_PROMPT = "Explain the purpose of FlockMTL.";

    std::shared_ptr<MockProvider> mock_provider;

    void SetUp() override;
    void TearDown() override;

    // Helper to create struct types with common patterns
    static duckdb::LogicalType CreateModelStruct();
    static duckdb::LogicalType CreatePromptStruct();
    static duckdb::LogicalType CreateInputStruct(const std::vector<std::string>& field_names);

    // Generic helper to create struct types for validation tests
    static duckdb::LogicalType CreateGenericStruct(const std::vector<std::pair<std::string, duckdb::LogicalType>>& fields);

    static void SetStructStringData(duckdb::Vector& struct_vector, const std::vector<std::map<std::string, std::string>>& data);

    // Helper to setup a basic 2-argument chunk for testing
    void CreateBasicChunk(duckdb::DataChunk& chunk, size_t cardinality = 1);

    // Common test methods
    void TestValidateArguments();
    void TestOperationInvalidArguments();
    void TestOperationEmptyPrompt();

    // Virtual methods for function-specific logic
    virtual std::string GetExpectedResponse() const = 0;
    virtual nlohmann::json GetExpectedJsonResponse() const = 0;
    virtual std::string GetFunctionName() const = 0;
    virtual nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const = 0;
    virtual nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const = 0;
    virtual std::string FormatExpectedResult(const nlohmann::json& response) const = 0;
};

}// namespace flockmtl
