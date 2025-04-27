#include "flockmtl/custom_parser/tokenizer.hpp"

#include <cctype>
#include <iostream>
#include <stdexcept>

namespace flockmtl {

// Constructor
Tokenizer::Tokenizer(const std::string& query) : _query(query), _position(0) {}

// Skip whitespace
void Tokenizer::SkipWhitespace() {
    while (_position < static_cast<int>(_query.size()) && std::isspace(_query[_position])) {
        ++_position;
    }
}

std::string Tokenizer::GetQuery() { return _query; }

// Parse a string literal
Token Tokenizer::ParseStringLiteral() {
    if (_query[_position] != '\'') {
        throw std::runtime_error("String literal should start with a single quote.");
    }
    ++_position;
    int start = _position;
    while (_position < static_cast<int>(_query.size()) && _query[_position] != '\'') {
        ++_position;
    }
    if (_position == static_cast<int>(_query.size())) {
        throw std::runtime_error("Unterminated string literal.");
    }
    std::string value = _query.substr(start, _position - start);
    ++_position;// Move past the closing quote
    return {TokenType::STRING_LITERAL, value};
}

Token Tokenizer::ParseJson() {
    if (_query[_position] != '{') {
        throw std::runtime_error("JSON should start with a curly brace.");
    }
    auto start = _position++;
    auto brace_count = 1;
    while (_position < static_cast<int>(_query.size()) && brace_count > 0) {
        if (_query[_position] == '{') {
            ++brace_count;
        } else if (_query[_position] == '}') {
            --brace_count;
        }
        ++_position;
    }
    if (brace_count > 0) {
        throw std::runtime_error("Unterminated JSON.");
    }
    auto value = _query.substr(start, _position - start);
    return {TokenType::JSON, value};
}

// Parse a keyword (word made of letters)
Token Tokenizer::ParseKeyword() {
    auto start = _position;
    while (_position < static_cast<int>(_query.size()) &&
           (std::isalpha(_query[_position]) || _query[_position] == '_')) {
        ++_position;
    }
    auto value = _query.substr(start, _position - start);
    return {TokenType::KEYWORD, value};
}

// Parse a symbol (single character)
Token Tokenizer::ParseSymbol() {
    auto ch = _query[_position];
    ++_position;
    return {TokenType::SYMBOL, std::string(1, ch)};
}

// Parse a number (sequence of digits)
Token Tokenizer::ParseNumber() {
    auto start = _position;
    while (_position < static_cast<int>(_query.size()) && std::isdigit(_query[_position])) {
        ++_position;
    }
    auto value = _query.substr(start, _position - start);
    return {TokenType::NUMBER, value};
}

// Parse a parenthesis
Token Tokenizer::ParseParenthesis() {
    auto ch = _query[_position];
    ++_position;
    return {TokenType::PARENTHESIS, std::string(1, ch)};
}

// Get the next token from the input
Token Tokenizer::GetNextToken() {
    SkipWhitespace();
    if (_position >= static_cast<int>(_query.size())) {
        return {TokenType::END_OF_FILE, ""};
    }

    auto ch = _query[_position];
    if (ch == '\'') {
        return ParseStringLiteral();
    } else if (ch == '{') {
        return ParseJson();
    } else if (std::isalpha(ch)) {
        return ParseKeyword();
    } else if (ch == ';' || ch == ',') {
        return ParseSymbol();
    } else if (ch == '=') {
        return ParseSymbol();
    } else if (ch == '(' || ch == ')') {
        return ParseParenthesis();
    } else if (std::isdigit(ch)) {
        return ParseNumber();
    } else {
        return {TokenType::UNKNOWN, std::string(1, ch)};
    }
}

// Get the next token
Token Tokenizer::NextToken() { return GetNextToken(); }

// Convert TokenType to string
std::string TokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD:
            return "KEYWORD";
        case TokenType::STRING_LITERAL:
            return "STRING_LITERAL";
        case TokenType::JSON:
            return "JSON";
        case TokenType::SYMBOL:
            return "SYMBOL";
        case TokenType::NUMBER:
            return "NUMBER";
        case TokenType::PARENTHESIS:
            return "PARENTHESIS";
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";
        case TokenType::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

}// namespace flockmtl
