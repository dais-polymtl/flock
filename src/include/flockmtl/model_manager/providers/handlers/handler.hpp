#pragma once

#include <nlohmann/json.hpp>

namespace flockmtl {

class IModelProviderHandler {
public:
    enum class RequestType { Completion,
                             Embedding };

    virtual ~IModelProviderHandler() = default;
    // AddRequest: type distinguishes between completion and embedding (default: Completion)
    virtual void AddRequest(const nlohmann::json& json, RequestType type = RequestType::Completion) = 0;

    // CollectCompletions: process all as completions, then clear
    virtual std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") = 0;
    // CollectEmbeddings: process all as embeddings, then clear
    virtual std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") = 0;
};

}// namespace flockmtl
