#pragma once

#include <nlohmann/json.hpp>

namespace flockmtl {

class IModelProviderHandler {
public:
    virtual ~IModelProviderHandler() = default;
    virtual nlohmann::json CallComplete(const nlohmann::json& json, const std::string& contentType = "application/json") = 0;
    virtual nlohmann::json CallEmbedding(const nlohmann::json& json, const std::string& contentType = "application/json") = 0;
};

}// namespace flockmtl
