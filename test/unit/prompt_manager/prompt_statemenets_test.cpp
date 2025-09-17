#include "flock/custom_parser/query/prompt_parser.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace flock {

// Test CreatePromptStatement
TEST(PromptStatementTest, CreatePromptStatement_DefaultInitialization) {
    CreatePromptStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::CREATE_PROMPT);
    EXPECT_TRUE(stmt.catalog.empty());
    EXPECT_TRUE(stmt.prompt_name.empty());
    EXPECT_TRUE(stmt.prompt.empty());
}

// Test DeletePromptStatement
TEST(PromptStatementTest, DeletePromptStatement_DefaultInitialization) {
    DeletePromptStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::DELETE_PROMPT);
    EXPECT_TRUE(stmt.prompt_name.empty());
}

// Test UpdatePromptScopeStatement
TEST(PromptStatementTest, UpdatePromptScopeStatement_DefaultInitialization) {
    UpdatePromptScopeStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::UPDATE_PROMPT_SCOPE);
    EXPECT_TRUE(stmt.catalog.empty());
    EXPECT_TRUE(stmt.prompt_name.empty());
}

// Test UpdatePromptStatement
TEST(PromptStatementTest, UpdatePromptStatement_DefaultInitialization) {
    UpdatePromptStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::UPDATE_PROMPT);
    EXPECT_TRUE(stmt.prompt_name.empty());
    EXPECT_TRUE(stmt.new_prompt.empty());
}

// Test GetPromptStatement
TEST(PromptStatementTest, GetPromptStatement_DefaultInitialization) {
    GetPromptStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::GET_PROMPT);
    EXPECT_TRUE(stmt.prompt_name.empty());
}

// Test GetAllPromptStatement
TEST(PromptStatementTest, GetAllPromptStatement_DefaultInitialization) {
    GetAllPromptStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::GET_ALL_PROMPT);
}

}// namespace flock