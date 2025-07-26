#include "flockmtl/functions/batch_response_builder.hpp"

namespace flockmtl {

std::vector<nlohmann::json> CastVectorOfStructsToJson(const duckdb::Vector& struct_vector, const int size) {
    std::vector<nlohmann::json> vector_json;
    for (auto i = 0; i < size; i++) {
        nlohmann::json json;
        for (auto j = 0; j < static_cast<int>(duckdb::StructType::GetChildCount(struct_vector.GetType())); j++) {
            const auto key = duckdb::StructType::GetChildName(struct_vector.GetType(), j);
            auto value = duckdb::StructValue::GetChildren(struct_vector.GetValue(i))[j];
            if (key == "batch_size") {
                if (value.GetTypeMutable() != duckdb::LogicalType::INTEGER) {
                    throw std::runtime_error("Expected 'batch_size' to be an integer.");
                }
                json[key] = value.GetValue<int>();
            } else {
                json[key] = value.ToString();
            }
        }
        vector_json.push_back(json);
    }
    return vector_json;
}

}// namespace flockmtl
