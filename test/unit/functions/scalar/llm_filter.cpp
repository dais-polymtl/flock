#include "flockmtl/functions/scalar/llm_filter.hpp"
#include "llm_function_test_base.hpp"

namespace flockmtl {

class LLMFilterTest : public LLMFunctionTestBase<LlmFilter> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "true";
    static constexpr const char* EXPECTED_FALSE_RESPONSE = "false";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {true}}};
    }

    std::string GetFunctionName() const override {
        return "llm_filter";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (const auto& response: responses) {
            // Convert string response to boolean for filter responses
            bool filter_result = (response == "True" || response == "true" || response == "1");
            expected_response["items"].push_back(filter_result);
        }
        return expected_response;
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < input_count; i++) {
            // Alternate between true and false for testing
            expected_response["items"].push_back(i % 2 == 0);
        }
        return expected_response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items") && response["items"].is_array() && !response["items"].empty()) {
            bool result = response["items"][0].get<bool>();
            return result ? "true" : "false";
        }
        return response.dump();
    }
};

// Test llm_filter with SQL queries - new API
TEST_F(LLMFilterTest, LLMFilterBasicUsage) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this sentiment positive?', 'context_columns': [{'data': text}]}) AS filter_result FROM unnest(['I love this product!']) as tbl(text);");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, LLMFilterWithMultipleRows) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this a valid email address?', 'context_columns': [{'data': email}]}) AS filter_result FROM unnest(['test@example.com', 'invalid-email', 'user@domain.org']) as tbl(email);");
    ASSERT_EQ(results->RowCount(), 3);
    // Check first result
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMFilterTest, Operation_BatchProcessing) {
    const nlohmann::json expected_response = {{"items", {true, false}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this review positive?', 'context_columns': [{'data': review}]}) AS result FROM unnest(['Great product!', 'Terrible quality']) as tbl(review);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);
    EXPECT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 10;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this content spam?', 'context_columns': [{'data': content}]}) AS result FROM range(" + std::to_string(input_count) + ") AS t(i), unnest(['Content item ' || i::TEXT]) as tbl(content);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), input_count);

    // Verify the first few results match expected
    for (size_t i = 0; i < std::min(input_count, size_t(5)); i++) {
        auto result_value = results->GetValue(0, i).GetValue<std::string>();
        auto expected_value = expected_response["items"][i].get<bool>() ? "true" : "false";
        EXPECT_EQ(result_value, expected_value);
    }
}

}// namespace flockmtl
