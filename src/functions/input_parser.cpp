#include "flock/functions/input_parser.hpp"

#include "duckdb/common/operator/cast_operators.hpp"

namespace flock {

// Helper function to validate and clean context column, handling NULL values
static void ValidateAndCleanContextColumn(nlohmann::json& column, const std::initializer_list<const char*>& allowed_keys) {
    std::string column_type = "";
    bool has_type = false;
    bool has_transcription_model = false;

    for (const auto& key: allowed_keys) {
        if (key != std::string("data")) {
            bool key_exists = column.contains(key);
            bool is_null = key_exists && column[key].get<std::string>() == "NULL";

            if (key == std::string("type") && key_exists && !is_null) {
                column_type = column[key].get<std::string>();
                has_type = true;
            } else if (key == std::string("transcription_model") && key_exists && !is_null) {
                has_transcription_model = true;
            } else if (!key_exists || is_null) {
                column.erase(key);
            }
        }
    }

    // Validate transcription_model is only used with audio type
    if (has_transcription_model && column_type != "audio") {
        std::string type_display = has_type ? column_type : "tabular";
        throw std::runtime_error(duckdb_fmt::format("Argument 'transcription_model' is not supported for data type '{}'. It can only be used with type 'audio'.", type_display));
    }

    // Validate that audio type requires transcription_model
    if (has_type && column_type == "audio" && !has_transcription_model) {
        throw std::runtime_error("Argument 'transcription_model' is required when type is 'audio'.");
    }
}

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
                    auto allowed_keys = {"name", "data", "type", "detail", "transcription_model"};
                    for (const auto& key: context_column_json.items()) {
                        if (std::find(std::begin(allowed_keys), std::end(allowed_keys), key.key()) == std::end(allowed_keys)) {
                            throw std::runtime_error(duckdb_fmt::format("Unexpected key in 'context_columns': {}", key.key()));
                        }
                    }

                    auto required_keys = {"data"};
                    for (const auto& key: required_keys) {
                        if (!context_column_json.contains(key) || (key != "data" && context_column_json[key].get<std::string>() == "NULL")) {
                            throw std::runtime_error(duckdb_fmt::format("Expected 'context_columns' to contain key: {}", key));
                        }
                    }

                    if (struct_json.contains("context_columns") && struct_json["context_columns"].size() == context_columns.size()) {
                        struct_json["context_columns"][context_column_idx]["data"].push_back(context_column_json["data"]);
                    } else {
                        struct_json["context_columns"].push_back(context_column_json);
                        ValidateAndCleanContextColumn(struct_json["context_columns"][context_column_idx], allowed_keys);
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

}// namespace flock
