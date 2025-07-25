#include "flockmtl/functions/scalar/llm_complete.hpp"
#include "llm_function_test_base.hpp"

namespace flockmtl {

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

TEST_F(LLMCompleteJsonTest, LLMCompleteJsonWithoutInputColumns) {
    nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'Explain the purpose of FlockMTL.'}) AS flockmtl_purpose;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_response["items"][0].dump());
}

TEST_F(LLMCompleteJsonTest, LLMCompleteJsonWithInputColumns) {
    const nlohmann::json expected_response = {{"items", {nlohmann::json::parse(R"({"capital": "Ottawa", "country": "Canada"})")}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'What is the capital of France?'}, {'input': 'France'}) AS flockmtl_capital;");
    ASSERT_EQ(results->RowCount(), 1);
    auto expected_json = expected_response["items"][0];
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_json.dump());
}

TEST_F(LLMCompleteJsonTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMCompleteJsonTest, Operation_TwoArguments_SimplePrompt) {
    nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    CreateBasicChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", TEST_PROMPT}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], expected_response["items"][0].dump());
}

TEST_F(LLMCompleteJsonTest, Operation_ThreeArguments_BatchProcessing) {
    const std::vector<std::string> responses = {"response 1", "response 2"};
    const nlohmann::json expected_response = PrepareExpectedResponseForBatch(responses);
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"variable1", "variable2"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(2);

    // Set data for batch processing
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", TEST_PROMPT}}});
    SetStructStringData(chunk.data[2], {{{"variable1", "Hello"}, {"variable2", "World"}},
                                        {{"variable1", "Good"}, {"variable2", "Morning"}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 2);
    std::vector<std::string> expected_strings;
    for (const auto& item: expected_response["items"]) {
        expected_strings.push_back(item.dump());
    }
    EXPECT_EQ(results, expected_strings);
}

TEST_F(LLMCompleteJsonTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

TEST_F(LLMCompleteJsonTest, Operation_EmptyPrompt_HandlesGracefully) {
    TestOperationEmptyPrompt();
}

TEST_F(LLMCompleteJsonTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"text"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(input_count);

    // Set model and prompt data
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", "Summarize the following text"}}});

    // Create large input dataset
    std::vector<std::map<std::string, std::string>> large_input;
    large_input.reserve(input_count);
    for (size_t i = 0; i < input_count; i++) {
        large_input.push_back({{"text", "Input text " + std::to_string(i)}});
    }

    SetStructStringData(chunk.data[2], large_input);

    const auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), input_count);
    std::vector<std::string> expected_strings;
    for (const auto& item: expected_response["items"]) {
        expected_strings.push_back(item.dump());
    }
    EXPECT_EQ(results, expected_strings);
}

}// namespace flockmtl
