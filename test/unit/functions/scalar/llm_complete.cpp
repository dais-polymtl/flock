#include "flockmtl/functions/scalar/llm_complete.hpp"
#include "llm_function_test_base.hpp"

namespace flockmtl {

class LLMCompleteTest : public LLMFunctionTestBase<LlmComplete> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "FlockMTL enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries.";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {EXPECTED_RESPONSE}}};
    }

    std::string GetFunctionName() const override {
        return "llm_complete";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"items", responses}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < input_count; i++) {
            expected_response["items"].push_back("response " + std::to_string(i));
        }
        return expected_response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items") && response["items"].is_array() && !response["items"].empty()) {
            return response["items"][0].get<std::string>();
        }
        return response.get<std::string>();
    }
};

// Test llm_complete with SQL queries
TEST_F(LLMCompleteTest, LLMCompleteWithoutInputColumns) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'Explain the purpose of FlockMTL.'}) AS flockmtl_purpose;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

TEST_F(LLMCompleteTest, LLMCompleteWithInputColumns) {
    const nlohmann::json expected_response = {{"items", {"The capital of Canada is Ottawa."}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'What is the capital of France?'}, {'input': 'France'}) AS flockmtl_capital;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_response["items"][0]);
}

TEST_F(LLMCompleteTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMCompleteTest, Operation_TwoArguments_SimplePrompt) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    duckdb::DataChunk chunk;
    CreateBasicChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", TEST_PROMPT}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], GetExpectedResponse());
}

TEST_F(LLMCompleteTest, Operation_ThreeArguments_BatchProcessing) {
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
    EXPECT_EQ(results, expected_response["items"]);
}

TEST_F(LLMCompleteTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

TEST_F(LLMCompleteTest, Operation_EmptyPrompt_HandlesGracefully) {
    TestOperationEmptyPrompt();
}

TEST_F(LLMCompleteTest, Operation_LargeInputSet_ProcessesCorrectly) {
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
    EXPECT_EQ(results, expected_response["items"]);
}

}// namespace flockmtl