#pragma once
#include "flockmtl/core/common.hpp"
#include "flockmtl/model_manager/model.hpp"
#include "flockmtl/prompt_manager/prompt_manager.hpp"
#include <nlohmann/json.hpp>

namespace flockmtl {

std::vector<nlohmann::json> CastVectorOfStructsToJson(const duckdb::Vector& struct_vector, int size);

}// namespace flockmtl
