#include "flock/functions/scalar/llm_complete.hpp"
#include "llm_function_test_base.hpp"

namespace flock {

class LLMCompleteJsonTest : public LLMFunctionTestBase<LlmComplete> {
protected:
    static constexpr const char* EXPECTED_JSON_RESPONSE = R"({"items": [{"explanation": "FlockMTL enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries."}]})";

    std::string GetExpectedResponse() const override {
        return EXPECTED_JSON_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(EXPECTED_JSON_RESPONSE);
    }

    std::string GetFunctionName() const override {
        return "llm_complete";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        nlohmann::json tuples = nlohmann::json::array();
        for (const auto& response: responses) {
            tuples.push_back(nlohmann::json::parse(R"({"response": ")" + response + R"("})"));
        }
        return nlohmann::json{{"items", tuples}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < input_count; i++) {
            expected_response["items"].push_back(nlohmann::json::parse(R"({"response": "response )" + std::to_string(i) + R"("})"));
        }
        return expected_response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items") && response["items"].is_array() && !response["items"].empty()) {
            return response["items"][0].dump();
        }
        return response.dump();
    }
};

// Test llm_complete with JSON responses using new SQL API
TEST_F(LLMCompleteJsonTest, LLMCompleteJsonBasicUsage) {
    nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Explain the purpose of FlockMTL.'}) AS flock_purpose;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_response["items"][0].dump());
}

TEST_F(LLMCompleteJsonTest, LLMCompleteJsonWithInputColumns) {
    const nlohmann::json expected_response = {{"items", {nlohmann::json::parse(R"({"capital": "Ottawa", "country": "Canada"})")}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'What is the capital of', 'context_columns': [{'data': country}]}) AS flock_capital FROM unnest(['Canada']) as tbl(country);");
    ASSERT_EQ(results->RowCount(), 1);
    auto expected_json = expected_response["items"][0];
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_json.dump());
}

TEST_F(LLMCompleteJsonTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMCompleteJsonTest, Operation_BatchProcessing) {
    const nlohmann::json expected_response = {{"items", {nlohmann::json::parse(R"({"response": "First response"})"), nlohmann::json::parse(R"({"response": "Second response"})")}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Describe the product', 'context_columns': [{'data': product}]}) AS result FROM unnest(['Product A', 'Product B']) as tbl(product);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);

    auto result_value = results->GetValue(0, 0).GetValue<std::string>();
    EXPECT_EQ(result_value, expected_response["items"][0].dump());
}

}// namespace flock
