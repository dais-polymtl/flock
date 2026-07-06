#pragma once

#include "flock/model_manager/repository.hpp"
#include <mutex>
#include <optional>

namespace flock {

class ModelUsageLimiter {
public:
    ModelUsageLimiter() = default;

    // Accumulates provider-reported token usage and throws UsageLimitExceededError
    // when any configured limit is exceeded.
    void RecordUsage(int64_t prompt_tokens, int64_t completion_tokens, const std::optional<UsageLimit>& limit);

    TotalUsage GetUsage() const;

    void Reset();

    bool IsLimitExceeded(const UsageLimit& limit) const;

    void ThrowIfLimitExceeded(const UsageLimit& limit) const;

private:
    void CheckLimit(const TotalUsage& usage, const UsageLimit& limit) const;

    mutable std::mutex mutex_;
    TotalUsage usage_;
};

}// namespace flock
