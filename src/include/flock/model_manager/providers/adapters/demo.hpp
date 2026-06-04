#pragma once

#include "flock/model_manager/providers/provider.hpp"

namespace flock {

class DemoProvider : public IProvider {
public:
    explicit DemoProvider(const ModelDetails& model_details) : IProvider(model_details) {}

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type,
                             const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
    void AddTranscriptionRequest(const nlohmann::json& audio_files) override;
    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") override;
    std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") override;
    std::vector<nlohmann::json> CollectTranscriptions(const std::string& contentType = "multipart/form-data") override;

private:
    struct CompletionRequest {
        int num_output_tuples;
        OutputType output_type;
    };

    std::vector<CompletionRequest> completion_queue_;
    std::vector<std::string> embedding_inputs_;
    std::vector<nlohmann::json> transcription_files_;
    
};

}// namespace flock
