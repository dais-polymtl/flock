#include "flock/custom_parser/query_parser.hpp"

#include "duckdb/main/materialized_query_result.hpp"
#include "flock/core/common.hpp"
#include "flock/core/config.hpp"

#include <sstream>
#include <stdexcept>

namespace flock {

// Format a DuckDB value for SQL (escape strings, handle NULLs)
std::string FormatValueForSQL(const duckdb::Value& value) {
    if (value.IsNull()) {
        return "NULL";
    }
    auto str = value.ToString();
    // Escape single quotes by doubling them
    std::string escaped;
    escaped.reserve(str.length() + 10);
    for (char c: str) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped += c;
        }
    }
    return "'" + escaped + "'";
}

// Format query results as VALUES clause: SELECT * FROM VALUES (...)
std::string FormatResultsAsValues(duckdb::unique_ptr<duckdb::QueryResult> result) {
    if (!result) {
        return "SELECT * FROM (VALUES (NULL)) AS empty_result WHERE FALSE";
    }

    // Cast to MaterializedQueryResult to access GetValue and RowCount
    auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();

    if (materialized_result.RowCount() == 0) {
        return "SELECT * FROM (VALUES (NULL)) AS empty_result WHERE FALSE";
    }

    std::ostringstream values_stream;
    auto column_count = result->ColumnCount();

    // Get column names
    std::vector<std::string> column_names;
    column_names.reserve(column_count);
    for (idx_t col = 0; col < column_count; col++) {
        column_names.push_back(result->ColumnName(col));
    }

    // Format each row as VALUES tuple
    for (idx_t row = 0; row < materialized_result.RowCount(); row++) {
        if (row > 0) {
            values_stream << ", ";
        }
        values_stream << "(";
        for (idx_t col = 0; col < column_count; col++) {
            if (col > 0) {
                values_stream << ", ";
            }
            auto value = materialized_result.GetValue(col, row);
            values_stream << FormatValueForSQL(value);
        }
        values_stream << ")";
    }

    // Build column names for the VALUES clause
    std::ostringstream column_names_stream;
    for (size_t i = 0; i < column_names.size(); i++) {
        if (i > 0) {
            column_names_stream << ", ";
        }
        column_names_stream << "\"" << column_names[i] << "\"";
    }

    return duckdb_fmt::format("SELECT * FROM (VALUES {}) AS result({})",
                              values_stream.str(), column_names_stream.str());
}

// Execute a query with storage attachment and return formatted result for GET operations
std::string ExecuteGetQuery(const std::string& query, bool read_only) {
    auto con = Config::GetConnection();
    Config::StorageAttachmentGuard guard(con, read_only);
    auto result = con.Query(query);
    return FormatResultsAsValues(std::move(result));
}

// Execute a query with storage attachment and return status message for SET operations
std::string ExecuteSetQuery(const std::string& query, const std::string& success_message, bool read_only) {
    auto con = Config::GetConnection();
    Config::StorageAttachmentGuard guard(con, read_only);
    con.Query(query);
    return duckdb_fmt::format("SELECT '{}' AS status", success_message);
}


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
