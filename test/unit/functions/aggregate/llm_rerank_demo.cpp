#include "flock/core/config.hpp"
#include "flock/functions/aggregate/llm_rerank.hpp"
#include "flock/model_manager/model.hpp"
#include <nlohmann/json.hpp>
#include <gtest/gtest.h>

namespace flock {

TEST(LLMRerankDemoProviderTest, SupportsDemoProviderWithMaxAsyncOverrides) {
    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_rerank("
            "{'model_name': 'gpt-4o', 'model': 'demo', 'provider': 'demo', 'batch_size': 3, 'tuple_format': 'json', 'max_async_calls': 2, 'secret': {'api_key': 'demo'}}, "
            "{'prompt': 'Rank these products', 'context_columns': [{'data': description}]}) AS reranked_products FROM range(5) AS t(i), "
            "unnest(['Product ' || i::VARCHAR]) AS products(description);");

    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    auto reranked = nlohmann::json::parse(results->GetValue(0, 0).GetValue<std::string>());
    ASSERT_EQ(reranked.size(), 1);
    ASSERT_TRUE(reranked[0].contains("data"));
    ASSERT_EQ(reranked[0]["data"].size(), 5);
}

TEST(LLMRerankDemoProviderTest, DemoProviderRerankOutputsAreDeterministic) {
    nlohmann::json model_config = {
            {"model_name", "gpt-4o"},
            {"model", "demo"},
            {"provider", "demo"},
            {"batch_size", 8},
            {"secret", nlohmann::json::object()},
            {"tuple_format", "json"},
            {"max_async_calls", 1}};

    Model model(model_config);

    model.AddCompletionRequest("prompt", 3, OutputType::INTEGER);
    model.AddCompletionRequest("prompt", 3, OutputType::INTEGER);
    auto responses = model.CollectCompletions();

    ASSERT_EQ(responses.size(), 2);
    EXPECT_EQ(responses[0]["items"][0], 0);
    EXPECT_EQ(responses[0]["items"][1], 1);
    EXPECT_EQ(responses[0]["items"][2], 2);
    EXPECT_EQ(responses[1]["items"][0], 100);
}

}// namespace flock
