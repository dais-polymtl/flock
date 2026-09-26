#pragma once

#include "flock/model_manager/providers/handlers/typesafe.hpp"
#include "flock/model_manager/providers/provider.hpp"
#include <string>
#include <vector>

namespace flock {

// Provider for TypeSafe's System One API (Jev).
class TypeSafeProvider : public IProvider {
public:
    TypeSafeProvider(const ModelDetails& model_details, std::shared_ptr<ModelRateLimiter> rate_limiter = nullptr,
                     std::shared_ptr<ModelUsageLimiter> usage_limiter = nullptr)
        : IProvider(model_details, std::move(rate_limiter), std::move(usage_limiter)) {
        model_handler_ = std::make_unique<TypeSafeModelManager>(
                model_details_.secret["api_key"],
                model_details_.secret.count("base_url") ? model_details_.secret["base_url"] : std::string(), true,
                model_details_.model_name,
                model_details_.rate_limit, model_details_.usage_limit, rate_limiter_, usage_limiter_);
    }

    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) override;
    void AddEmbeddingRequest(const std::vector<std::string>& inputs) override;
    void AddTranscriptionRequest(const nlohmann::json& audio_files) override;

    bool AcceptsStructuredTuples() const override {
        return true;
    }
    void AddStructuredCompletionRequest(const StructuredCompletionRequest& request) override;
    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json") override;

private:
    // One entry per queued batch, in queue order.
    struct PendingBatch {
        size_t row_count;
        // llm_filter only.
        double threshold;
        std::vector<size_t> asked_rows;
    };

    std::vector<PendingBatch> pending_batches_;
};

}// namespace flock
