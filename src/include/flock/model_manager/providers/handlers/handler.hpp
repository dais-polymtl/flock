#pragma once

#include "flock/core/common.hpp"
#include <nlohmann/json.hpp>

namespace flock {

class IModelProviderHandler {
public:
    enum class RequestType { Completion,
                             Embedding,
                             Transcription };

    virtual ~IModelProviderHandler() = default;
    // AddRequest: type distinguishes between completion, embedding, and transcription (default: Completion)
    virtual void AddRequest(const nlohmann::json& json, RequestType type = RequestType::Completion) = 0;

    // CollectCompletions: process all as completions, then clear
    virtual std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") = 0;
    // CollectEmbeddings: process all as embeddings, then clear
    virtual std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") = 0;
    // CollectTranscriptions: process all transcriptions, then clear
    virtual std::vector<nlohmann::json> CollectTranscriptions(const std::string& contentType = "multipart/form-data") = 0;
};

}// namespace flock
