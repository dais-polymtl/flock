#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flockmtl {

class LLMLastTest : public LLMAggregateTestBase<LlmFirstOrLast> {
protected:
    // The LLM response (for mocking) - for llm_last, it should select the last index
    static constexpr const char* LLM_RESPONSE = R"({"selected": 0})";
    // The expected function output (selected data from the last position)
    static constexpr const char* EXPECTED_RESPONSE = R"({"product_description":"High-performance running shoes with advanced cushioning"})";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(LLM_RESPONSE);
    }

    std::string GetFunctionName() const override {
        return "llm_last";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::LAST;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"selected", static_cast<int>(responses.size() - 1)}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        return nlohmann::json{{"selected", static_cast<int>(input_count - 1)}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test llm_last with SQL queries without GROUP BY
TEST_F(LLMLastTest, LLMLastWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(GetExpectedJsonResponse()));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the least relevant detail for these products, based on their names and descriptions?'}, "
                                            "{'product_description': description}"
                                            ") AS last_product_feature FROM test_products;");

    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test llm_last with SQL queries with GROUP BY
TEST_F(LLMLastTest, LLMLastWithGroupBy) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(GetExpectedJsonResponse()));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the least relevant detail for these products, based on their names and descriptions?'}, "
                                            "{'product_description': description}"
                                            ") AS last_feature FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("product_description"));
        });
    }
}

// Test argument validation
TEST_F(LLMLastTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMLastTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

// Test operation with multiple input scenarios
TEST_F(LLMLastTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(expected_response));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the least relevant product information?'}, "
                                            "{'product_description': description}"
                                            ") AS last_relevant_info FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("product_description"));
        });
    }
}

// Test large input set processing
TEST_F(LLMLastTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .Times(100)
            .WillRepeatedly(::testing::Return(expected_response));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Select the last relevant product based on relevance'}, "
                                            "{'id': id, 'description': description}"
                                            ") AS last_relevant FROM test_large_dataset GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("description"));
        });
    }
}

}// namespace flockmtl
