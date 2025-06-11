#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flockmtl {

class LLMFirstTest : public LLMAggregateTestBase<LlmFirstOrLast> {
protected:
    // The LLM response (for mocking)
    static constexpr const char* LLM_RESPONSE = R"({"selected": 0})";
    // The expected function output (selected data)
    static constexpr const char* EXPECTED_RESPONSE = R"({"product_description":"High-performance running shoes with advanced cushioning"})";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(LLM_RESPONSE);
    }

    std::string GetFunctionName() const override {
        return "llm_first";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::FIRST;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"selected", 0}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        return nlohmann::json{{"selected", 0}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test llm_first with SQL queries without GROUP BY
TEST_F(LLMFirstTest, LLMFirstWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(GetExpectedJsonResponse()));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?'}, "
                                            "{'product_description': description}"
                                            ") AS first_product_feature FROM test_products;");

    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test llm_first with SQL queries with GROUP BY
TEST_F(LLMFirstTest, LLMFirstWithGroupBy) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(GetExpectedJsonResponse()));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?'}, "
                                            "{'product_description': description}"
                                            ") AS first_feature FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("product_description"));
        });
    }
}

// Test argument validation
TEST_F(LLMFirstTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMFirstTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

// Test operation with multiple input scenarios
TEST_F(LLMFirstTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(expected_response));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the most relevant product information?'}, "
                                            "{'product_description': description}"
                                            ") AS first_relevant_info FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("product_description"));
        });
    }
}

// Test large input set processing
TEST_F(LLMFirstTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(expected_response));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Select the first relevant product based on relevance'}, "
                                            "{'id': id, 'description': description}"
                                            ") AS first_relevant FROM test_large_dataset GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("description"));
        });
    }
}

}// namespace flockmtl
