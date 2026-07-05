#pragma once

#include "flock/model_manager/providers/provider.hpp"
#include "flock/model_manager/usage_limiter.hpp"

namespace flock {

class UsageLimitAwareProvider : public IProvider {
public:
    explicit UsageLimitAwareProvider(const ModelDetails& model_details,
                                     std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                                     std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : IProvider(model_details, std::move(rate_limiter), std::move(usage_limiter)) {}

    void SetTokensPerRequest(int prompt_tokens, int completion_tokens) {
        prompt_tokens_per_request_ = prompt_tokens;
        completion_tokens_per_request_ = completion_tokens;
    }

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

        if (usage_limiter_ != nullptr && model_details_.usage_limit.has_value()) {
            try {
                usage_limiter_->ThrowIfLimitExceeded(model_details_.model_name, *model_details_.usage_limit);
            } catch (const UsageLimitExceededError&) {
                collects_blocked_at_precheck++;
                pending_output_counts_.clear();
                throw;
            }
        }

        bool usage_limit_reached = false;
        std::vector<nlohmann::json> responses;
        responses.reserve(pending_output_counts_.size());
        for (const int num_output_tuples: pending_output_counts_) {
            if (usage_limiter_ != nullptr && model_details_.usage_limit.has_value() && !usage_limit_reached) {
                try {
                    usage_limiter_->RecordUsage(model_details_.model_name, prompt_tokens_per_request_,
                                                completion_tokens_per_request_, model_details_.usage_limit);
                } catch (const UsageLimitExceededError&) {
                    usage_limit_reached = true;
                }
            }

            nlohmann::json items = nlohmann::json::array();
            for (int j = 0; j < num_output_tuples; j++) {
                items.push_back("response-" + std::to_string(next_request_id_++));
            }
            responses.push_back({{"items", items}});
            requests_processed_count++;
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
    int requests_processed_count = 0;
    int collects_blocked_at_precheck = 0;

private:
    int prompt_tokens_per_request_ = 50;
    int completion_tokens_per_request_ = 10;
    int next_request_id_ = 0;
    std::vector<int> pending_output_counts_;
};

}// namespace flock
