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
                    if (struct_json.contains("context_columns") && static_cast<int>(struct_json["context_columns"].size()) > 0) {
                        struct_json["context_columns"][context_column_idx]["column"].push_back(context_column_json["column"]);
                    } else {
                        struct_json["context_columns"] = nlohmann::json::array();
                        struct_json["context_columns"].push_back(context_column_json);
                        struct_json["context_columns"][context_column_idx]["column"] = nlohmann::json::array();
                        struct_json["context_columns"][context_column_idx]["column"].push_back(context_column_json["column"]);
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
