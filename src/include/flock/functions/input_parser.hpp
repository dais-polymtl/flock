#pragma once
#include "flock/core/common.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/prompt_manager/prompt_manager.hpp"
#include <nlohmann/json.hpp>

namespace flock {

nlohmann::json CastVectorOfStructsToJson(const duckdb::Vector& struct_vector, int size);
nlohmann::json CastValueToJson(const duckdb::Value& value);

}// namespace flock
