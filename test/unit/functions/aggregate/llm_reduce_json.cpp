#include "flock/functions/aggregate/llm_reduce.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flock {

class LLMReduceJsonTest : public LLMAggregateTestBase<LlmReduce> {
protected:
    static constexpr const char* EXPECTED_JSON_RESPONSE = R"({"items": [{"summary": "A comprehensive summary of some products"}]})";

    std::string GetExpectedResponse() const override {
        return EXPECTED_JSON_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(EXPECTED_JSON_RESPONSE);
    }

    std::string GetFunctionName() const override {
        return "llm_reduce";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::REDUCE;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json::parse(responses[0]);
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json response;
        response["summary"] = "Summary of " + std::to_string(input_count) + " items processed";
        response["total_items"] = input_count;
        response["status"] = "processed";
        return response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test llm_reduce_json with SQL queries without GROUP BY - new API
TEST_F(LLMReduceJsonTest, LLMReduceJsonWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Summarize the following product descriptions as JSON with summary, key_themes, and product_count fields', 'context_columns': [{'data': description}]}"
                                            ") AS product_summary FROM VALUES "
                                            "('High-performance running shoes with advanced cushioning'), "
                                            "('Wireless noise-cancelling headphones for immersive audio'), "
                                            "('Smart fitness tracker with heart rate monitoring') AS products(description);");

    ASSERT_EQ(results->RowCount(), 1);
    const auto expected_json = GetExpectedJsonResponse()["items"][0].dump();
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_json);
}

// Test llm_reduce_json with SQL queries with GROUP BY - new API
TEST_F(LLMReduceJsonTest, LLMReduceJsonWithGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, " + GetFunctionName() + "("
                                                      "{'model_name': 'gpt-4o'}, "
                                                      "{'prompt': 'Summarize the following product descriptions as JSON with summary, key_themes, and product_count fields', 'context_columns': [{'data': description}]}"
                                                      ") AS description_summary FROM VALUES "
                                                      "('electronics', 'High-performance running shoes with advanced cushioning'), "
                                                      "('audio', 'Wireless noise-cancelling headphones for immersive audio'), "
                                                      "('fitness', 'Smart fitness tracker with heart rate monitoring') "
                                                      "AS products(category, description) GROUP BY category;");

    ASSERT_EQ(results->RowCount(), 3);
    const auto expected_json = GetExpectedJsonResponse()["items"][0].dump();
    ASSERT_EQ(results->GetValue(1, 0).GetValue<std::string>(), expected_json);
    ASSERT_EQ(results->GetValue(1, 1).GetValue<std::string>(), expected_json);
    ASSERT_EQ(results->GetValue(1, 2).GetValue<std::string>(), expected_json);
}

// Test argument validation
TEST_F(LLMReduceJsonTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMReduceJsonTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

// Test operation with multiple input scenarios for JSON output - new API
TEST_F(LLMReduceJsonTest, Operation_MultipleInputs_ProcessesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT name, " + GetFunctionName() + "("
                                                  "{'model_name': 'gpt-4o'}, "
                                                  "{'prompt': 'Analyze the following product information and return JSON with summary, categories, and insights', 'context_columns': [{'data': name}, {'data': description}]}"
                                                  ") AS comprehensive_analysis FROM VALUES "
                                                  "('Running Shoes', 'High-performance running shoes with advanced cushioning'), "
                                                  "('Headphones', 'Wireless noise-cancelling headphones for immersive audio'), "
                                                  "('Fitness Tracker', 'Smart fitness tracker with heart rate monitoring') "
                                                  "AS products(name, description) GROUP BY name;");

    ASSERT_EQ(results->RowCount(), 3);
    const auto expected_json = GetExpectedJsonResponse()["items"][0].dump();
    ASSERT_EQ(results->GetValue(1, 0).GetValue<std::string>(), expected_json);
    ASSERT_EQ(results->GetValue(1, 1).GetValue<std::string>(), expected_json);
    ASSERT_EQ(results->GetValue(1, 2).GetValue<std::string>(), expected_json);
}

// Test large input set processing with JSON output - new API
TEST_F(LLMReduceJsonTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(100);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(100)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT id, " + GetFunctionName() + "("
                                                "{'model_name': 'gpt-4o'}, "
                                                "{'prompt': 'Create a JSON summary of all product descriptions with summary, total_items, and status fields', 'context_columns': [{'data': id::VARCHAR}, {'data': 'Product description ' || id::VARCHAR}]}"
                                                ") AS large_json_summary FROM range(" +
            std::to_string(input_count) + ") AS t(id) GROUP BY id;");

    ASSERT_EQ(results->RowCount(), 100);
    for (size_t i = 0; i < input_count; i++) {
        const auto expected_json = GetExpectedJsonResponse()["items"][0].dump();
        ASSERT_EQ(results->GetValue(1, i).GetValue<std::string>(), expected_json);
    }
}

// Test JSON-specific functionality - ensure output is valid JSON - new API
TEST_F(LLMReduceJsonTest, Operation_ValidJsonOutput_ParsesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Return a JSON object with product analysis including summary and metadata', 'context_columns': [{'data': description}]}"
                                            ") AS json_analysis FROM VALUES "
                                            "('High-performance running shoes with advanced cushioning'), "
                                            "('Wireless noise-cancelling headphones for immersive audio'), "
                                            "('Smart fitness tracker with heart rate monitoring') AS products(description);");

    ASSERT_EQ(results->RowCount(), 1);
    const std::string result = results->GetValue(0, 0).GetValue<std::string>();

    // Verify the result is valid JSON by parsing it
    EXPECT_NO_THROW({
        nlohmann::json parsed = nlohmann::json::parse(result);
        EXPECT_TRUE(parsed.contains("summary"));
    });
}

// Test complex JSON structure output - new API
TEST_F(LLMReduceJsonTest, Operation_ComplexJsonStructure_HandlesCorrectly) {
    nlohmann::json complex_response;
    nlohmann::json item;
    item["metadata"] = {
            {"total_products", 3},
            {"analysis_timestamp", "2025-06-10T10:00:00Z"},
            {"version", "1.0"}};
    item["insights"] = {
            {"categories", nlohmann::json::array({"electronics", "fitness", "wearables"})},
            {"price_range", {{"min", 50}, {"max", 300}}},
            {"features", nlohmann::json::array({"wireless", "smart", "comfortable"})}};
    item["summary"] = "Comprehensive analysis of diverse product portfolio";

    complex_response["items"].push_back(item);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{complex_response}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o'}, "
                                            "{'prompt': 'Provide a detailed JSON analysis with nested metadata, insights, and summary', 'context_columns': [{'data': name}, {'data': description}]}"
                                            ") AS complex_analysis FROM VALUES "
                                            "('Running Shoes', 'High-performance running shoes with advanced cushioning'), "
                                            "('Headphones', 'Wireless noise-cancelling headphones for immersive audio'), "
                                            "('Fitness Tracker', 'Smart fitness tracker with heart rate monitoring') AS products(name, description);");

    ASSERT_EQ(results->RowCount(), 1);
    const std::string result = results->GetValue(0, 0).GetValue<std::string>();

    // Verify the complex JSON structure is preserved
    EXPECT_NO_THROW({
        nlohmann::json parsed = nlohmann::json::parse(result);
        EXPECT_TRUE(parsed.contains("metadata"));
        EXPECT_TRUE(parsed.contains("insights"));
        EXPECT_TRUE(parsed.contains("summary"));
        EXPECT_TRUE(parsed["metadata"].contains("total_products"));
        EXPECT_TRUE(parsed["insights"].contains("categories"));
        EXPECT_TRUE(parsed["insights"]["categories"].is_array());
    });
}

}// namespace flock
