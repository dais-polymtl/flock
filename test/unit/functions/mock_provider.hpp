#pragma once
#include "flock/model_manager/providers/provider.hpp"
#include <gmock/gmock.h>

namespace flock {

class MockProvider : public IProvider {
public:
    explicit MockProvider(const ModelDetails& model_details,
                          std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                          std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : IProvider(model_details, std::move(rate_limiter), std::move(usage_limiter)) {}

    MOCK_METHOD(void, AddCompletionRequest, (const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data), (override));
    MOCK_METHOD(void, AddEmbeddingRequest, (const std::vector<std::string>& inputs), (override));
    MOCK_METHOD(void, AddTranscriptionRequest, (const nlohmann::json& audio_files), (override));
    MOCK_METHOD(std::vector<nlohmann::json>, CollectCompletions, (const std::string& contentType), (override));
    MOCK_METHOD(std::vector<nlohmann::json>, CollectEmbeddings, (const std::string& contentType), (override));
    MOCK_METHOD(std::vector<nlohmann::json>, CollectTranscriptions, (const std::string& contentType), (override));
};

}// namespace flock
