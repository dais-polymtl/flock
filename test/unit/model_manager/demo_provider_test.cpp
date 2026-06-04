#include "flock/core/config.hpp"
#include "flock/model_manager/model.hpp"
#include "nlohmann/json.hpp"
#include <gtest/gtest.h>

namespace flock {

TEST(DemoProviderTest, DemoCompletionReturnsChunkedSyntheticResponses) {
    nlohmann::json model_config = {
            {"model_name", "gpt-4o"},
            {"model", "demo"},
            {"provider", "demo"},
            {"secret", nlohmann::json::object()},
            {"batch_size", 4},
            {"max_async_calls", 2},
            {"tuple_format", "json"}};

    Model model(model_config);

    for (int i = 0; i < 5; ++i) {
        model.AddCompletionRequest("prompt " + std::to_string(i), 1, OutputType::STRING, {});
    }

    auto responses = model.CollectCompletions();
    ASSERT_EQ(responses.size(), 5);

    const auto& first = responses[0]["items"][0];
    const auto& second = responses[1]["items"][0];
    const auto& third = responses[2]["items"][0];
    const auto& fourth = responses[3]["items"][0];
    const auto& fifth = responses[4]["items"][0];

    EXPECT_EQ(first, "demo_chunk_0_request_0_output_0");
    EXPECT_EQ(second, "demo_chunk_0_request_1_output_0");
    EXPECT_EQ(third, "demo_chunk_1_request_2_output_0");
    EXPECT_EQ(fourth, "demo_chunk_1_request_3_output_0");
    EXPECT_EQ(fifth, "demo_chunk_2_request_4_output_0");

    const auto& fifth_chunk = responses[4]["items"][0].get<std::string>();
    EXPECT_NE(fifth_chunk.find("chunk_2"), std::string::npos);
}

TEST(DemoProviderTest, DemoEmbeddingsReturnSyntheticVectors) {
    nlohmann::json model_config = {
            {"model_name", "gpt-4o"},
            {"model", "demo"},
            {"provider", "demo"},
            {"secret", nlohmann::json::object()},
            {"batch_size", 4},
            {"max_async_calls", 1},
            {"tuple_format", "json"}};

    Model model(model_config);

    model.AddEmbeddingRequest({"one", "two", "three"});
    auto response = model.CollectEmbeddings();

    ASSERT_EQ(response.size(), 3);
    for (auto& value : response) {
        ASSERT_TRUE(value.is_array());
        ASSERT_EQ(value.size(), 4);
    }
    EXPECT_EQ(response[0][0], 0.1);
    EXPECT_EQ(response[0][1], 0.2);
    EXPECT_EQ(response[0][2], 0.3);
    EXPECT_EQ(response[0][3], 0.4);
}

TEST(DemoProviderTest, DemoTranscriptionReturnsSyntheticText) {
    nlohmann::json model_config = {
            {"model_name", "gpt-4o"},
            {"model", "demo"},
            {"provider", "demo"},
            {"secret", nlohmann::json::object()},
            {"batch_size", 4},
            {"tuple_format", "json"}};

    Model model(model_config);

    model.AddTranscriptionRequest({"audio_one.wav", "audio_two.wav"});
    auto response = model.CollectTranscriptions();

    ASSERT_EQ(response.size(), 2);
    EXPECT_EQ(response[0]["text"], "demo transcribed: audio_one.wav");
    EXPECT_EQ(response[1]["text"], "demo transcribed: audio_two.wav");
}

}// namespace flock
