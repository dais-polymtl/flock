#pragma once

#include "../mock_provider.hpp"
#include "flockmtl/core/config.hpp"
#include "flockmtl/functions/aggregate/aggregate.hpp"
#include "flockmtl/model_manager/model.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace flockmtl {

// Base template class for LLM aggregate function tests
template<typename FunctionClass>
class LLMAggregateTestBase : public ::testing::Test {
protected:
    // Common test constants
    static constexpr const char* DEFAULT_MODEL = "gpt-4o";
    static constexpr const char* TEST_PROMPT = "Summarize the following data";

    std::shared_ptr<MockProvider> mock_provider;

    void SetUp() override {
        auto con = Config::GetConnection();
        con.Query(" CREATE SECRET ("
                  "       TYPE OPENAI,"
                  "    API_KEY 'your-api-key');");

        mock_provider = std::make_shared<MockProvider>(ModelDetails{});
        Model::SetMockProvider(mock_provider);
    }

    void TearDown() override {
        Model::ResetMockProvider();
        mock_provider = nullptr;
    }

    // Common test methods that can be used by derived classes if needed
    void TestValidateArguments() {
        // Test with invalid SQL syntax - missing required arguments (new API expects 2 arguments)
        auto con = Config::GetConnection();
        const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}) AS result FROM VALUES ('test') AS tbl(data);");
        ASSERT_TRUE(results->HasError()) << "Expected error for missing arguments, but query succeeded";
    }

    void TestOperationInvalidArguments() {
        // Test with invalid arguments using SQL API (new API)
        auto con = Config::GetConnection();
        const auto results = con.Query("SELECT " + GetFunctionName() + "('invalid_arg') AS result FROM VALUES ('test') AS tbl(data);");
        ASSERT_TRUE(results->HasError()) << "Expected error for invalid arguments, but query succeeded";
    }

    // Virtual methods for function-specific logic
    virtual std::string GetExpectedResponse() const = 0;
    virtual nlohmann::json GetExpectedJsonResponse() const = 0;
    virtual std::string GetFunctionName() const = 0;
    virtual AggregateFunctionType GetFunctionType() const = 0;
    virtual nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const = 0;
    virtual nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const = 0;
    virtual std::string FormatExpectedResult(const nlohmann::json& response) const = 0;
};

}// namespace flockmtl
