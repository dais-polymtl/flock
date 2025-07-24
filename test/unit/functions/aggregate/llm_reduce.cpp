#include "flockmtl/functions/aggregate/llm_reduce.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flockmtl {

class LLMReduceTest : public LLMAggregateTestBase<LlmReduce> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "A comprehensive summary of running shoes, wireless headphones, and smart watches, featuring advanced technology and user-friendly designs for active lifestyles.";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {EXPECTED_RESPONSE}}};
    }

    std::string GetFunctionName() const override {
        return "llm_reduce";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::REDUCE;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"items", {responses[0]}}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        return nlohmann::json{{"items", {"Summary of " + std::to_string(input_count) + " items processed"}}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items")) {
            return response["items"][0].get<std::string>();
        }
        return response.get<std::string>();
    }
};

// Test llm_reduce with SQL queries without GROUP BY
TEST_F(LLMReduceTest, LLMReduceWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Summarize the following product descriptions'}, "
                                            "{'description': description}"
                                            ") AS product_summary FROM test_products;");

    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test llm_reduce with SQL queries with GROUP BY
TEST_F(LLMReduceTest, LLMReduceWithGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Summarize the following product descriptions'}, "
                                            "{'description': description}"
                                            ") AS description_summary FROM test_products GROUP BY description;");

    ASSERT_EQ(results->RowCount(), 3);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(0, 1).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(0, 2).GetValue<std::string>(), GetExpectedResponse());
}

// Test argument validation
TEST_F(LLMReduceTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMReduceTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

// Test operation with multiple input scenarios
TEST_F(LLMReduceTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Summarize the following product information'}, "
                                            "{'name': name, 'description': description}"
                                            ") AS comprehensive_summary FROM test_products GROUP BY name;");

    ASSERT_EQ(results->RowCount(), 3);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(0, 1).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(0, 2).GetValue<std::string>(), GetExpectedResponse());
}

// Test large input set processing
TEST_F(LLMReduceTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(100);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(100)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Summarize all product descriptions'}, "
                                            "{'id': id, 'description': description}"
                                            ") AS large_summary FROM test_large_dataset GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (size_t i = 0; i < input_count; i++) {
        ASSERT_EQ(results->GetValue(0, i).GetValue<std::string>(), FormatExpectedResult(expected_response));
    }
}

}// namespace flockmtl
