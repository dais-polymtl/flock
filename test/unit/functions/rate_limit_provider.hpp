#pragma once

#include "flock/model_manager/providers/provider.hpp"
#include "flock/model_manager/rate_limiter.hpp"

namespace flock {

class RateLimitAwareProvider : public IProvider {
public:
    explicit RateLimitAwareProvider(const ModelDetails& model_details,
                                    std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                                    std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : IProvider(model_details, std::move(rate_limiter), std::move(usage_limiter)) {}

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type,
                              const nlohmann::json& media_data) override {
        (void) prompt;
        (void) output_type;
        (void) media_data;
        pending_output_counts_.push_back(num_output_tuples);
    }

    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override { (void) inputs; }

    void AddTranscriptionRequest(const nlohmann::json& audio_files) override { (void) audio_files; }

    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") override {
        (void) contentType;
        collect_call_count++;

        const int request_count = static_cast<int>(pending_output_counts_.size());
        last_batch_request_count = request_count;

        if (model_details_.rate_limit.has_value() && request_count > 0 && rate_limiter_ != nullptr) {
            rate_limiter_->WaitForBatchIfNeeded(static_cast<size_t>(request_count), model_details_.rate_limit.value());
        }

        std::vector<nlohmann::json> responses;
        responses.reserve(pending_output_counts_.size());
        for (const int num_output_tuples: pending_output_counts_) {
            nlohmann::json items = nlohmann::json::array();
            for (int i = 0; i < num_output_tuples; i++) {
                items.push_back("ok");
            }
            responses.push_back({{"items", items}});
        }

        pending_output_counts_.clear();
        return responses;
    }

    std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json") override {
        (void) contentType;
        return {};
    }

    std::vector<nlohmann::json> CollectTranscriptions(const std::string& contentType = "multipart/form-data") override {
        (void) contentType;
        return {};
    }

    int collect_call_count = 0;
    int last_batch_request_count = 0;

private:
    std::vector<int> pending_output_counts_;
};

}// namespace flock
