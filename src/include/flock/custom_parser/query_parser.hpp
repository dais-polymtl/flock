#pragma once

#include "flock/core/common.hpp"
#include "flock/core/config.hpp"
#include "flock/custom_parser/query/model_parser.hpp"
#include "flock/custom_parser/query/prompt_parser.hpp"
#include "flock/custom_parser/query_statements.hpp"
#include "flock/custom_parser/tokenizer.hpp"

#include "fmt/format.h"

namespace flock {

// Forward declarations for query execution utilities
std::string FormatValueForSQL(const duckdb::Value& value);
std::string FormatResultsAsValues(duckdb::unique_ptr<duckdb::QueryResult> result);
std::string ExecuteGetQuery(const std::string& query, bool read_only);
std::string ExecuteSetQuery(const std::string& query, const std::string& success_message, bool read_only);

// Template function for executing queries with storage attachment
template<typename Func>
std::string ExecuteQueryWithStorage(Func&& query_func, bool read_only) {
    auto con = Config::GetConnection();
    Config::StorageAttachmentGuard guard(con, read_only);
    return query_func(con);
}

class QueryParser {
public:
    std::string ParseQuery(const std::string& query);
    std::string ParsePromptOrModel(Tokenizer tokenizer, const std::string& query);

private:
    std::unique_ptr<QueryStatement> statement;
};

}// namespace flock
