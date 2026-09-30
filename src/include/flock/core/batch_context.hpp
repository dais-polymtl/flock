#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace flock {

// A column's own name, or "COLUMN N" counting unnamed columns only.
inline std::vector<std::string> ContextColumnNames(const nlohmann::json& columns) {
    std::vector<std::string> names;
    names.reserve(columns.size());
    auto unnamed_index = 1u;
    for (const auto& column: columns) {
        if (column.contains("name") && column["name"].is_string()) {
            names.push_back(column["name"].get<std::string>());
        } else {
            // if the column does not have a specific name we just call it "COLUMN 1...N"
            names.push_back("COLUMN " + std::to_string(unnamed_index++));
        }
    }
    return names;
}

// The context columns of one batch of rows, before anything has rendered them into a prompt.
class BatchContext {
public:
    explicit BatchContext(nlohmann::json columns)
        : columns_(std::move(columns)), column_names_(ContextColumnNames(columns_)) {}

    size_t RowCount() const {
        if (columns_.empty() || !columns_[0].contains("data")) {
            return 0;
        }
        return columns_[0]["data"].size();
    }

    // One row as {column name: value}.
    nlohmann::json Row(const size_t row) const {
        auto values = nlohmann::json::object();
        for (size_t i = 0; i < columns_.size(); i++) {
            const auto& column = columns_[i];
            if (column.contains("data") && row < column["data"].size()) {
                values[column_names_[i]] = column["data"][row];
            }
        }
        return values;
    }

    const nlohmann::json& Columns() const {
        return columns_;
    }

private:
    nlohmann::json columns_;
    std::vector<std::string> column_names_;
};

}// namespace flock
