#include "flock/custom_parser/query_parser.hpp"

#include "flock/core/common.hpp"

#include <sstream>
#include <stdexcept>

namespace flock {

std::string QueryParser::ParseQuery(const std::string& query) {
    Tokenizer tokenizer(query);

    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD ||
        (value != "CREATE" && value != "DELETE" && value != "UPDATE" && value != "GET")) {
        throw std::runtime_error(duckdb_fmt::format("Unknown keyword: {}", token.value));
    }

    return ParsePromptOrModel(tokenizer, query);
}

inline std::string QueryParser::ParsePromptOrModel(Tokenizer tokenizer, const std::string& query) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type == TokenType::KEYWORD && (value == "MODEL" || value == "MODELS")) {
        ModelParser model_parser;
        model_parser.Parse(query, statement);
        return model_parser.ToSQL(*statement);
    } else if (token.type == TokenType::KEYWORD && (value == "PROMPT" || value == "PROMPTS")) {
        PromptParser prompt_parser;
        prompt_parser.Parse(query, statement);
        return prompt_parser.ToSQL(*statement);
    } else if (token.type == TokenType::KEYWORD && (value == "GLOBAL" || value == "LOCAL")) {
        return ParsePromptOrModel(tokenizer, query);
    } else {
        throw std::runtime_error(duckdb_fmt::format("Unknown keyword: {}", token.value));
    }
}

}// namespace flock
