#include "flock/core/config.hpp"
#include "flock/functions/scalar/llm_complete.hpp"
#include <gtest/gtest.h>

namespace flock {

TEST(LlmCompleteDemoProviderTest, SupportsDemoProviderInSqlAndAsyncOverrides) {
    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_complete("
            "{'model_name': 'gpt-4o', 'model': 'demo', 'provider': 'demo', 'batch_size': 4, 'max_async_calls': 2, 'secret': {'api_key': 'demo'}}, "
            "{'prompt': 'What is this?', 'context_columns': [{'data': product}]}) AS result "
            "FROM range(5) AS t(i), unnest(['Product ' || i::VARCHAR]) AS products(product);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 5);

    for (idx_t i = 0; i < results->RowCount(); ++i) {
        const auto value = results->GetValue(0, i).GetValue<std::string>();
        ASSERT_FALSE(value.empty());
        EXPECT_NE(value.find("demo_chunk_"), std::string::npos);
    }
}

}// namespace flock
