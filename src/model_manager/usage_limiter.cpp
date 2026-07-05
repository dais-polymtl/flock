#include "flock/model_manager/usage_limiter.hpp"

#include "flock/model_manager/providers/provider.hpp"

namespace flock {

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

void ModelUsageLimiter::RecordUsage(int64_t prompt_tokens, int64_t completion_tokens,
                                    const std::optional<UsageLimit>& limit) {
    if (!limit.has_value() || !limit->HasAnyLimit()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    usage_.prompt_tokens += prompt_tokens;
    usage_.completion_tokens += completion_tokens;
    CheckLimit(usage_, *limit);
}

TotalUsage ModelUsageLimiter::GetUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return usage_;
}

void ModelUsageLimiter::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    usage_ = {};
}

bool ModelUsageLimiter::IsLimitExceeded(const UsageLimit& limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        CheckLimit(usage_, limit);
        return false;
    } catch (const UsageLimitExceededError&) {
        return true;
    }
}

void ModelUsageLimiter::ThrowIfLimitExceeded(const UsageLimit& limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    CheckLimit(usage_, limit);
}

}// namespace flock
