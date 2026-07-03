#include "flock/model_manager/usage_limiter.hpp"

#include "flock/model_manager/providers/provider.hpp"

namespace flock {

ModelUsageLimiter& ModelUsageLimiter::Instance() {
    static ModelUsageLimiter instance;
    return instance;
}

void ModelUsageLimiter::CheckLimit(const TotalUsage& usage, const UsageLimit& limit) const {
    if (limit.prompt_tokens_limit.has_value() && usage.prompt_tokens >= *limit.prompt_tokens_limit) {
        throw UsageLimitExceededError("prompt_tokens", usage.prompt_tokens, *limit.prompt_tokens_limit);
    }
    if (limit.completion_tokens_limit.has_value() && usage.completion_tokens >= *limit.completion_tokens_limit) {
        throw UsageLimitExceededError("completion_tokens", usage.completion_tokens, *limit.completion_tokens_limit);
    }
    if (limit.total_tokens_limit.has_value() && usage.total_tokens() >= *limit.total_tokens_limit) {
        throw UsageLimitExceededError("total_tokens", usage.total_tokens(), *limit.total_tokens_limit);
    }
}

void ModelUsageLimiter::RecordUsage(const std::string& model_name, int64_t prompt_tokens, int64_t completion_tokens,
                                    const std::optional<UsageLimit>& limit) {
    if (model_name.empty() || !limit.has_value() || !limit->HasAnyLimit()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& usage = usage_by_model_[model_name];
    usage.prompt_tokens += prompt_tokens;
    usage.completion_tokens += completion_tokens;
    CheckLimit(usage, *limit);
}

TotalUsage ModelUsageLimiter::GetUsageForTesting(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = usage_by_model_.find(model_name);
    if (it == usage_by_model_.end()) {
        return {};
    }
    return it->second;
}

void ModelUsageLimiter::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    usage_by_model_.clear();
}

}// namespace flock
