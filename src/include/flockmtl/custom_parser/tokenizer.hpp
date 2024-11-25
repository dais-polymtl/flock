#pragma once

#include <string>

namespace flockmtl {

// Define token types
enum class TokenType { KEYWORD, STRING_LITERAL, JSON, SYMBOL, NUMBER, PARENTHESIS, END_OF_FILE, UNKNOWN };

// Token structure
struct Token {
    TokenType type;
    std::string value;
};

// Tokenizer class declaration
class Tokenizer {
public:
    Tokenizer(const std::string &query);
    Token NextToken();
    std::string GetQuery();

private:
    std::string query_;
    int position_;

    void SkipWhitespace();
    Token ParseStringLiteral();
    Token ParseKeyword();
    Token ParseSymbol();
    Token ParseNumber();
    Token ParseParenthesis();
    Token ParseJson();
    Token GetNextToken();
};

// Utility function to convert TokenType to string
std::string TokenTypeToString(TokenType type);

} // namespace flockmtl
