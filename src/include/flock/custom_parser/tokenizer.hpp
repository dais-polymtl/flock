#pragma once

#include "flock/core/common.hpp"

namespace flock {

// Define token types
enum class TokenType { KEYWORD,
                       STRING_LITERAL,
                       JSON,
                       SYMBOL,
                       NUMBER,
                       PARENTHESIS,
                       COMMENT,
                       END_OF_FILE,
                       UNKNOWN };

// Token structure
struct Token {
    TokenType type;
    std::string value;
};

// Tokenizer class declaration
class Tokenizer {
public:
    Tokenizer(const std::string& query);
    Token NextToken();
    std::string GetQuery();

    void SkipWhitespace();
    Token ParseStringLiteral();
    Token ParseKeyword();
    Token ParseSymbol();
    Token ParseNumber();
    Token ParseParenthesis();
    Token ParseComment();
    Token ParseJson();
    Token GetNextToken();

private:
    std::string _query;
    int _position;
};

// Utility function to convert TokenType to string
std::string TokenTypeToString(TokenType type);

}// namespace flock
