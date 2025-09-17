#include "flock/custom_parser/query/model_parser.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace flock {
// Test CreateModelStatement
TEST(ModelStatementTest, CreateModelStatement_DefaultInitialization) {
    CreateModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::CREATE_MODEL);
    EXPECT_TRUE(stmt.catalog.empty());
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.model.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
    EXPECT_TRUE(stmt.model_args.is_null() || stmt.model_args.empty());
}

// Test DeleteModelStatement
TEST(ModelStatementTest, DeleteModelStatement_DefaultInitialization) {
    DeleteModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::DELETE_MODEL);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
}

// Test UpdateModelScopeStatement
TEST(ModelStatementTest, UpdateModelScopeStatement_DefaultInitialization) {
    UpdateModelScopeStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::UPDATE_MODEL_SCOPE);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.catalog.empty());
}

// Test UpdateModelStatement
TEST(ModelStatementTest, UpdateModelStatement_DefaultInitialization) {
    UpdateModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::UPDATE_MODEL);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.new_model.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
    EXPECT_TRUE(stmt.new_model_args.is_null() || stmt.new_model_args.empty());
}

// Test GetModelStatement
TEST(ModelStatementTest, GetModelStatement_DefaultInitialization) {
    GetModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::GET_MODEL);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
}

// Test GetAllModelStatement
TEST(ModelStatementTest, GetAllModelStatement_DefaultInitialization) {
    GetAllModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::GET_ALL_MODEL);
}

}// namespace flock
