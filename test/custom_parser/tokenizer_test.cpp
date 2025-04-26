#include "flockmtl/custom_parser/tokenizer.hpp"
#include "gtest/gtest.h"

using namespace flockmtl;

TEST(TokenizerTest, SkipWhitespace) {
    Tokenizer tokenizer("   test");
    tokenizer.SkipWhitespace();
    EXPECT_EQ(tokenizer.GetQuery().substr(tokenizer.GetQuery().find_first_not_of(' ')), "test");
}

TEST(TokenizerTest, ParseStringLiteral) {
    Tokenizer tokenizer("'hello'");
    Token token = tokenizer.ParseStringLiteral();
    EXPECT_EQ(token.type, TokenType::STRING_LITERAL);
    EXPECT_EQ(token.value, "hello");
}

TEST(TokenizerTest, ParseJson) {
    Tokenizer tokenizer("{\"key\":\"value\"}");
    Token token = tokenizer.ParseJson();
    EXPECT_EQ(token.type, TokenType::JSON);
    EXPECT_EQ(token.value, "{\"key\":\"value\"}");
}

TEST(TokenizerTest, ParseKeyword) {
    Tokenizer tokenizer("keyword");
    Token token = tokenizer.ParseKeyword();
    EXPECT_EQ(token.type, TokenType::KEYWORD);
    EXPECT_EQ(token.value, "keyword");
}

TEST(TokenizerTest, ParseSymbol) {
    Tokenizer tokenizer(",");
    Token token = tokenizer.ParseSymbol();
    EXPECT_EQ(token.type, TokenType::SYMBOL);
    EXPECT_EQ(token.value, ",");
}

TEST(TokenizerTest, ParseNumber) {
    Tokenizer tokenizer("12345");
    Token token = tokenizer.ParseNumber();
    EXPECT_EQ(token.type, TokenType::NUMBER);
    EXPECT_EQ(token.value, "12345");
}

TEST(TokenizerTest, ParseParenthesis) {
    Tokenizer tokenizer("(");
    Token token = tokenizer.ParseParenthesis();
    EXPECT_EQ(token.type, TokenType::PARENTHESIS);
    EXPECT_EQ(token.value, "(");
}

TEST(TokenizerTest, GetNextToken_EndOfFile) {
    Tokenizer tokenizer("");
    Token token = tokenizer.GetNextToken();
    EXPECT_EQ(token.type, TokenType::END_OF_FILE);
    EXPECT_EQ(token.value, "");
}

TEST(TokenizerTest, GetNextToken_Unknown) {
    Tokenizer tokenizer("@");
    Token token = tokenizer.GetNextToken();
    EXPECT_EQ(token.type, TokenType::UNKNOWN);
    EXPECT_EQ(token.value, "@");
}