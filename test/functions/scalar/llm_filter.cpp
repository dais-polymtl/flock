#include "flockmtl/functions/scalar/llm_filter.hpp"
#include "llm_function_test_base.hpp"

namespace flockmtl {

class LLMFilterTest : public LLMFunctionTestBase<LlmFilter> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "true";
    static constexpr const char* EXPECTED_FALSE_RESPONSE = "false";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {true}}};
    }

    std::string GetFunctionName() const override {
        return "llm_filter";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (const auto& response: responses) {
            // Convert string response to boolean for filter responses
            bool filter_result = (response == "True" || response == "true" || response == "1");
            expected_response["items"].push_back(filter_result);
        }
        return expected_response;
    }

    // Helper method to handle vector of booleans
    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<bool>& responses) const {
        nlohmann::json expected_response = {{"items", {}}};
        for (const auto& response: responses) {
            expected_response["items"].push_back(response);
        }
        return expected_response;
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < input_count; i++) {
            // Alternate between true and false for testing
            expected_response["items"].push_back(i % 2 == 0);
        }
        return expected_response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items") && response["items"].is_array() && !response["items"].empty()) {
            bool result = response["items"][0].get<bool>();
            return result ? "true" : "false";
        }
        return response.dump();
    }
};

// Test llm_filter with SQL queries
TEST_F(LLMFilterTest, LLMFilterWithInputColumns) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(::testing::AtLeast(1));
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'Is positive?'}, {'input': i}) AS filter FROM range(1) AS tbl(i);");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, LLMFilterWithMultipleInputs) {
    const nlohmann::json expected_response = {{"items", {false}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'Is positive?'}, {'input1': i, 'input2': i}) AS filter FROM range(1) AS tbl(i);");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "false");
}

TEST_F(LLMFilterTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMFilterTest, Operation_ThreeArguments_RequiredStructure) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"sentiment_text"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(1);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", "Does this text express positive sentiment?"}}});
    SetStructStringData(chunk.data[2], {{{"sentiment_text", "I love this product!"}}});

    auto results = LlmFilter::Operation(chunk, ExecutionMode::ASYNC);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], FormatExpectedResult(expected_response));
}

TEST_F(LLMFilterTest, Operation_BatchProcessing) {
    const std::vector<bool> filter_responses = {true, false};
    const nlohmann::json expected_response = PrepareExpectedResponseForBatch(filter_responses);
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"review_text", "rating"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(2);

    // Set data for batch processing
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", "Is this review positive based on text and rating?"}}});
    SetStructStringData(chunk.data[2], {{{"review_text", "Great product"}, {"rating", "5"}},
                                        {{"review_text", "Terrible quality"}, {"rating", "1"}}});

    auto results = LlmFilter::Operation(chunk, ExecutionMode::ASYNC);

    EXPECT_EQ(results.size(), 2);
    std::vector<std::string> expected_results;
    for (const auto& item: expected_response["items"]) {
        bool bool_val = item.get<bool>();
        expected_results.push_back(bool_val ? "true" : "false");
    }
    EXPECT_EQ(results, expected_results);
}

TEST_F(LLMFilterTest, Operation_BatchProcessing_StringVector) {
    // Test with vector of strings (for compatibility with base class interface)
    const std::vector<std::string> filter_responses = {"true", "false"};
    const nlohmann::json expected_response = PrepareExpectedResponseForBatch(filter_responses);
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"text_content"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(2);

    // Set data for batch processing
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", "Filter spam content"}}});
    SetStructStringData(chunk.data[2], {{{"text_content", "Great offer!"}},
                                        {{"text_content", "Click here now!"}}});

    auto results = LlmFilter::Operation(chunk, ExecutionMode::ASYNC);

    EXPECT_EQ(results.size(), 2);
    std::vector<std::string> expected_results;
    for (const auto& item: expected_response["items"]) {
        bool bool_val = item.get<bool>();
        expected_results.push_back(bool_val ? "true" : "false");
    }
    EXPECT_EQ(results, expected_results);
}

TEST_F(LLMFilterTest, Operation_InvalidArguments_ThrowsException) {
    TestOperationInvalidArguments();
}

TEST_F(LLMFilterTest, Operation_EmptyPrompt_HandlesGracefully) {
    TestOperationEmptyPrompt();
}

TEST_F(LLMFilterTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"content"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(input_count);

    // Set model and prompt data
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", "Is this content spam?"}}});

    // Create large input dataset
    std::vector<std::map<std::string, std::string>> large_input;
    large_input.reserve(input_count);
    for (size_t i = 0; i < input_count; i++) {
        large_input.push_back({{"content", "Content item " + std::to_string(i)}});
    }

    SetStructStringData(chunk.data[2], large_input);

    const auto results = LlmFilter::Operation(chunk, ExecutionMode::ASYNC);

    EXPECT_EQ(results.size(), input_count);
    std::vector<std::string> expected_strings;
    for (const auto& item: expected_response["items"]) {
        expected_strings.push_back(item.get<bool>() ? "true" : "false");
    }
    EXPECT_EQ(results, expected_strings);
}

TEST_F(LLMFilterTest, Operation_TwoArguments_ThrowsException) {
    // llm_filter requires exactly 3 arguments, unlike llm_complete which can take 2 or 3
    duckdb::DataChunk chunk;
    CreateBasicChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", TEST_PROMPT}}});

    EXPECT_THROW(LlmFilter::Operation(chunk, ExecutionMode::ASYNC);, std::runtime_error);
}

}// namespace flockmtl
