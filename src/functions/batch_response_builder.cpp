#include "flockmtl/functions/batch_response_builder.hpp"

#include "duckdb/common/operator/cast_operators.hpp"

namespace flockmtl {

nlohmann::json CastVectorOfStructsToJson(const duckdb::Vector& struct_vector, const int size) {
    nlohmann::json struct_json;

    for (auto i = 0; i < size; i++) {
        for (auto j = 0; j < static_cast<int>(duckdb::StructType::GetChildCount(struct_vector.GetType())); j++) {
            const auto key = duckdb::StructType::GetChildName(struct_vector.GetType(), j);
            auto value = duckdb::StructValue::GetChildren(struct_vector.GetValue(i))[j];
            if (key == "context_columns") {
                if (value.GetTypeMutable().id() != duckdb::LogicalTypeId::LIST) {
                    throw std::runtime_error("Expected 'context_columns' to be a list.");
                }

                auto context_columns = duckdb::ListValue::GetChildren(value);
                for (auto context_column_idx = 0; context_column_idx < static_cast<int>(context_columns.size()); context_column_idx++) {
                    auto context_column = context_columns[context_column_idx];
                    auto context_column_json = CastVectorOfStructsToJson(duckdb::Vector(context_column), 1);
                    auto allowed_keys = {"name", "data", "type", "detail"};
                    for (const auto& key: context_column_json.items()) {
                        if (std::find(std::begin(allowed_keys), std::end(allowed_keys), key.key()) == std::end(allowed_keys)) {
                            throw std::runtime_error(duckdb_fmt::format("Unexpected key in 'context_columns': {}", key.key()));
                        }
                    }
                    auto required_keys = {"data"};
                    for (const auto& key: required_keys) {
                        if (!context_column_json.contains(key) || context_column_json[key].get<std::string>() == "NULL") {
                            throw std::runtime_error(duckdb_fmt::format("Expected 'context_columns' to contain key: {}", key));
                        }
                    }
                    if (struct_json.contains("context_columns") && struct_json["context_columns"].size() == context_columns.size()) {
                        struct_json["context_columns"][context_column_idx]["data"].push_back(context_column_json["data"]);
                    } else {
                        struct_json["context_columns"].push_back(context_column_json);
                        for (const auto& key: allowed_keys) {
                            if (key != "data" && (!struct_json["context_columns"][context_column_idx].contains(key) ||
                                                  struct_json["context_columns"][context_column_idx][key].get<std::string>() == "NULL")) {
                                struct_json["context_columns"][context_column_idx].erase(key);
                            }
                        }
                        struct_json["context_columns"][context_column_idx]["data"] = nlohmann::json::array();
                        struct_json["context_columns"][context_column_idx]["data"].push_back(context_column_json["data"]);
                    }
                }
            } else if (key == "batch_size") {
                if (value.GetTypeMutable() != duckdb::LogicalType::INTEGER) {
                    throw std::runtime_error("Expected 'batch_size' to be an integer.");
                }
                struct_json[key] = value.GetValue<int>();
            } else {
                struct_json[key] = value.ToString();
            }
        }
    }
    return struct_json;
}

}// namespace flockmtl
