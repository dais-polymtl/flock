#include "flockmtl/functions/aggregate/llm_rerank.hpp"
#include "llm_aggregate_function_test_base.hpp"
#include <numeric>

namespace flockmtl {

class LLMRerankTest : public LLMAggregateTestBase<LlmRerank> {
protected:
    // The LLM response (for mocking) - returns ranking indices
    static constexpr const char* LLM_RESPONSE_WITHOUT_GROUP_BY = R"({"items":[0, 1, 2]})";
    static constexpr const char* LLM_RESPONSE_WITH_GROUP_BY = R"({"items":[0]})";
    // The expected function output (reranked data as JSON array)
    static constexpr const char* EXPECTED_RESPONSE = R"([{"product_description":"High-performance running shoes with advanced cushioning"},{"product_description":"Professional business shoes"},{"product_description":"Casual sneakers for everyday wear"}])";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(LLM_RESPONSE_WITHOUT_GROUP_BY);
    }

    std::string GetFunctionName() const override {
        return "llm_rerank";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::RERANK;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        std::vector<int> ranking_indices(responses.size());
        std::iota(ranking_indices.begin(), ranking_indices.end(), 0);
        return nlohmann::json{{"items", ranking_indices}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        std::vector<int> ranking_indices(input_count);
        std::iota(ranking_indices.begin(), ranking_indices.end(), 0);
        return nlohmann::json{{"items", ranking_indices}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test llm_rerank with SQL queries without GROUP BY - new API
TEST_F(LLMRerankTest, LLMRerankWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Rank these products by their relevance and quality based on descriptions', 'context_columns': [{'data': description}]}"
                                            ") AS reranked_products FROM VALUES "
                                            "('High-performance running shoes with advanced cushioning'), "
                                            "('Professional business shoes'), "
                                            "('Casual sneakers for everyday wear') AS products(description);");

    ASSERT_EQ(results->RowCount(), 1);
    EXPECT_NO_THROW({
        nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, 0).GetValue<std::string>());
        EXPECT_EQ(parsed.size(), 1);
        EXPECT_TRUE(parsed[0].contains("data"));
        EXPECT_EQ(parsed[0]["data"].size(), 3);
    });
}

// Test llm_rerank with SQL queries with GROUP BY - new API
TEST_F(LLMRerankTest, LLMRerankWithGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{nlohmann::json::parse(LLM_RESPONSE_WITH_GROUP_BY)}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, " + GetFunctionName() + "("
                                                      "{'model_name': 'gpt-4o'}, "
                                                      "{'prompt': 'Rank these products by their relevance and quality based on descriptions', 'context_columns': [{'data': description}]}"
                                                      ") AS reranked_products FROM VALUES "
                                                      "('electronics', 'High-performance running shoes with advanced cushioning'), "
                                                      "('audio', 'Professional business shoes'), "
                                                      "('fitness', 'Casual sneakers for everyday wear') "
                                                      "AS products(category, description) GROUP BY category;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_EQ(parsed.size(), 1);
            EXPECT_TRUE(parsed[0].contains("data"));
            EXPECT_EQ(parsed[0]["data"].size(), 1);
        });
    }
}

// Test argument validation
TEST_F(LLMRerankTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMRerankTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

// Test operation with multiple input scenarios - new API
TEST_F(LLMRerankTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = nlohmann::json::parse(LLM_RESPONSE_WITH_GROUP_BY);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, " + GetFunctionName() + "("
                                                      "{'model_name': 'gpt-4o'}, "
                                                      "{'prompt': 'Rank products by relevance to customer preferences', 'context_columns': [{'data': id::TEXT}, {'data': description}]}"
                                                      ") AS reranked_products FROM VALUES "
                                                      "('electronics', 1, 'High-performance running shoes with advanced cushioning'), "
                                                      "('audio', 2, 'Professional business shoes'), "
                                                      "('fitness', 3, 'Casual sneakers for everyday wear') "
                                                      "AS products(category, id, description) GROUP BY category;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_EQ(parsed.size(), 2);
            EXPECT_TRUE(parsed[0].contains("data"));
            EXPECT_EQ(parsed[0]["data"].size(), 1);
        });
    }
}

// Test large input set processing - new API
TEST_F(LLMRerankTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = nlohmann::json::parse(LLM_RESPONSE_WITH_GROUP_BY);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(100);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(100)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT id, " + GetFunctionName() + "("
                                                "{'model_name': 'gpt-4o'}, "
                                                "{'prompt': 'Rerank products by relevance and importance', 'context_columns': [{'data': id::TEXT}, {'data': 'Product description ' || id::TEXT}]}"
                                                ") AS reranked_products FROM range(" +
            std::to_string(input_count) + ") AS t(id) GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_EQ(parsed.size(), 2);
            EXPECT_TRUE(parsed[0].contains("data"));
            EXPECT_EQ(parsed[0]["data"].size(), 1);
        });
    }

    ::testing::Mock::AllowLeak(mock_provider.get());
}

}// namespace flockmtl
