#include "flockmtl/custom_parser/query/prompt_parser.hpp"
#include "flockmtl/custom_parser/tokenizer.hpp"
#include "gtest/gtest.h"
#include <memory>

using namespace flockmtl;

/**************************************************
 *                 Create Prompt                  *
 **************************************************/

TEST(PromptParserTest, ParseCreatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseCreateGlobalPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseCreateLocalPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(PromptParserTest, ParseInvalidCreatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("CREATE PROMPT ('test_prompt')", statement), std::runtime_error);
}

/**************************************************
 *                 Delete Prompt                  *
 **************************************************/

TEST(PromptParserTest, ParseDeletePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE PROMPT 'test_prompt';", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseInvalidDeletePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("DELETE PROMPT", statement), std::runtime_error);
}

/**************************************************
 *                 Update Prompt                  *
 **************************************************/

TEST(PromptParserTest, ParseUpdatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT ('test_prompt', 'Updated prompt text.')", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseUpdateScopeToGlobal) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO GLOBAL;", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseUpdateScopeToLocal) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO LOCAL;", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(PromptParserTest, ParseInvalidUpdatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("UPDATE PROMPT ('test_prompt')", statement), std::runtime_error);
}

/**************************************************
 *                   Get Prompt                   *
 **************************************************/

TEST(PromptParserTest, ParseGetPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPT 'test_prompt';", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseInvalidGetPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("GET PROMPT", statement), std::runtime_error);
}

/**************************************************
 *                Get All Prompts                 *
 **************************************************/

TEST(PromptParserTest, ParseGetAllPrompts) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPTS;", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllPromptStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(PromptParserTest, ParseEmptyQuery) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("", statement), std::runtime_error);
}