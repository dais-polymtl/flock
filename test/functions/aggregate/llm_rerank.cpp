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

// Test llm_rerank with SQL queries without GROUP BY
TEST_F(LLMRerankTest, LLMRerankWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(GetExpectedJsonResponse()));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Rank these products by their relevance and quality based on descriptions'}, "
                                            "{'product_description': description}"
                                            ") AS reranked_products FROM test_products;");

    ASSERT_EQ(results->RowCount(), 1);
    EXPECT_NO_THROW({
        nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, 0).GetValue<std::string>());
        EXPECT_EQ(parsed.size(), 2);
        EXPECT_TRUE(parsed[0].contains("product_description"));
    });
}

// Test llm_rerank with SQL queries with GROUP BY
TEST_F(LLMRerankTest, LLMRerankWithGroupBy) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_, ::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(nlohmann::json::parse(LLM_RESPONSE_WITH_GROUP_BY)));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Rank these products by their relevance and quality based on descriptions'}, "
                                            "{'product_description': description}"
                                            ") AS reranked_products FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.is_array());
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

// Test operation with multiple input scenarios
TEST_F(LLMRerankTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = nlohmann::json::parse(LLM_RESPONSE_WITH_GROUP_BY);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_, ::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(expected_response));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Rank products by relevance to customer preferences'}, "
                                            "{'id': id, 'product_description': description}"
                                            ") AS reranked_products FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.is_array());
        });
    }
}

// Test large input set processing
TEST_F(LLMRerankTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Return(expected_response));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Rerank products by relevance and importance'}, "
                                            "{'id': id, 'description': description}"
                                            ") AS reranked_products FROM test_large_dataset GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, i).GetValue<std::string>());
            EXPECT_TRUE(parsed.is_array());
            EXPECT_TRUE(parsed[0].contains("description"));
        });
    }

    ::testing::Mock::AllowLeak(mock_provider.get());
}

}// namespace flockmtl
