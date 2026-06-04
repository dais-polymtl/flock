#include "flock/model_manager/providers/adapters/demo.hpp"

namespace flock {

void DemoProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type,
                                       const nlohmann::json& media_data) {
    (void)prompt;
    (void)media_data;
    completion_queue_.push_back({num_output_tuples, output_type});
}

void DemoProvider::AddEmbeddingRequest(const std::vector<std::string>& inputs) {
    embedding_inputs_.insert(embedding_inputs_.end(), inputs.begin(), inputs.end());
}

void DemoProvider::AddTranscriptionRequest(const nlohmann::json& audio_files) {
    if (audio_files.is_array()) {
        for (const auto& file : audio_files) {
            transcription_files_.push_back(file);
        }
    }
}

std::vector<nlohmann::json> DemoProvider::CollectCompletions(const std::string& contentType) {
    (void)contentType;
    std::vector<nlohmann::json> responses;
    responses.reserve(completion_queue_.size());

    const size_t max_batch_size = std::max<size_t>(1, static_cast<size_t>(model_details_.max_async_calls));
    size_t request_index = 0;
    for (size_t batch_start = 0; batch_start < completion_queue_.size(); batch_start += max_batch_size) {
        const size_t batch_end = std::min(batch_start + max_batch_size, completion_queue_.size());
        const int chunk_id = static_cast<int>(batch_start / max_batch_size);

        for (size_t idx = batch_start; idx < batch_end; ++idx) {
            const auto& request = completion_queue_[idx];
            nlohmann::json items = nlohmann::json::array();

            for (int i = 0; i < request.num_output_tuples; ++i) {
                const std::string label = "demo_chunk_" + std::to_string(chunk_id) + "_request_" + std::to_string(request_index++) +
                                          "_output_" + std::to_string(i);

                if (request.output_type == OutputType::INTEGER) {
                    items.push_back(static_cast<int>(idx * 100 + i));
                } else if (request.output_type == OutputType::BOOL) {
                    items.push_back(((idx + i) % 2) == 0);
                } else if (request.output_type == OutputType::OBJECT) {
                    items.push_back({
                            {"demo_chunk", chunk_id},
                            {"demo_request", static_cast<int>(idx)},
                            {"demo_output", i},
                            {"label", label}});
                } else {
                    items.push_back(label);
                }
            }

            responses.push_back({{"items", items}});
        }
    }

    completion_queue_.clear();
    return responses;
}

std::vector<nlohmann::json> DemoProvider::CollectEmbeddings(const std::string& contentType) {
    (void)contentType;
    std::vector<nlohmann::json> responses;
    responses.reserve(embedding_inputs_.size());

    auto embedding = nlohmann::json::array({0.1, 0.2, 0.3, 0.4});

    const size_t max_batch_size = std::max<size_t>(1, static_cast<size_t>(model_details_.max_async_calls));
    for (size_t batch_start = 0; batch_start < embedding_inputs_.size(); batch_start += max_batch_size) {
        const size_t batch_end = std::min(batch_start + max_batch_size, embedding_inputs_.size());
        for (size_t i = batch_start; i < batch_end; ++i) {
            responses.push_back(embedding);
        }
    }

    embedding_inputs_.clear();
    return responses;
}

std::vector<nlohmann::json> DemoProvider::CollectTranscriptions(const std::string& contentType) {
    (void)contentType;
    std::vector<nlohmann::json> responses;
    responses.reserve(transcription_files_.size());

    for (size_t i = 0; i < transcription_files_.size(); ++i) {
        const auto& entry = transcription_files_[i];
        if (entry.is_string()) {
            responses.push_back({{"text", "demo transcribed: " + entry.get<std::string>()}});
        } else {
            responses.push_back({{"text", "demo transcribed: " + entry.dump()}});
        }
    }

    transcription_files_.clear();
    return responses;
}

}// namespace flock
