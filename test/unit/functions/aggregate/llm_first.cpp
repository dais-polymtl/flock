#include "flockmtl/functions/aggregate/llm_first_or_last.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flockmtl {

class LLMFirstTest : public LLMAggregateTestBase<LlmFirstOrLast> {
protected:
    // The LLM response (for mocking)
    static constexpr const char* LLM_RESPONSE = R"({"items":[0]})";
    // The expected function output (selected data)
    static constexpr const char* EXPECTED_RESPONSE = R"({"data":["High-performance running shoes with advanced cushioning"]})";

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
        return nlohmann::json{{"items", {0}}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        return nlohmann::json{{"items", {0}}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test llm_first with SQL queries without GROUP BY - new API
TEST_F(LLMFirstTest, LLMFirstWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?', 'context_columns': [{'data': description}]}"
                                            ") AS first_product_feature FROM VALUES "
                                            "('High-performance running shoes with advanced cushioning'), "
                                            "('Wireless noise-cancelling headphones for immersive audio'), "
                                            "('Smart fitness tracker with heart rate monitoring') AS products(description);");

    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test llm_first with SQL queries with GROUP BY - new API
TEST_F(LLMFirstTest, LLMFirstWithGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, " + GetFunctionName() + "("
                                                      "{'model_name': 'gpt-4o'}, "
                                                      "{'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?', 'context_columns': [{'data': description}]}"
                                                      ") AS first_feature FROM VALUES "
                                                      "('electronics', 'High-performance running shoes with advanced cushioning'), "
                                                      "('audio', 'Wireless noise-cancelling headphones for immersive audio'), "
                                                      "('fitness', 'Smart fitness tracker with heart rate monitoring') "
                                                      "AS products(category, description) GROUP BY category;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("data"));
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

// Test operation with multiple input scenarios - new API
TEST_F(LLMFirstTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, " + GetFunctionName() + "("
                                                      "{'model_name': 'gpt-4o'}, "
                                                      "{'prompt': 'What is the most relevant product information?', 'context_columns': [{'data': description}]}"
                                                      ") AS first_relevant_info FROM VALUES "
                                                      "('electronics', 'High-performance running shoes with advanced cushioning'), "
                                                      "('audio', 'Wireless noise-cancelling headphones for immersive audio'), "
                                                      "('fitness', 'Smart fitness tracker with heart rate monitoring') "
                                                      "AS products(category, description) GROUP BY category;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("data"));
        });
    }
}

// Test large input set processing - new API
TEST_F(LLMFirstTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(100);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(100)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT id, " + GetFunctionName() + "("
                                                "{'model_name': 'gpt-4o'}, "
                                                "{'prompt': 'Select the first relevant product based on relevance', 'context_columns': [{'data': 'Product description ' || id::TEXT}]}"
                                                ") AS first_relevant FROM range(" +
            std::to_string(input_count) + ") AS t(id) GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.contains("data"));
        });
    }
}

}// namespace flockmtl
