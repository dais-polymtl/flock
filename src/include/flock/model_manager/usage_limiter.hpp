#pragma once

#include "flock/model_manager/repository.hpp"
#include <mutex>
#include <string>
#include <unordered_map>

namespace flock {

class ModelUsageLimiter {
public:
    ModelUsageLimiter() = default;

    // Accumulates provider-reported token usage for `model_name` and throws
    // UsageLimitExceededError when any configured limit is exceeded.
    void RecordUsage(const std::string& model_name, int64_t prompt_tokens, int64_t completion_tokens,
                     const std::optional<UsageLimit>& limit);

    TotalUsage GetUsage(const std::string& model_name) const;

    void Reset();

    bool IsLimitExceeded(const std::string& model_name, const UsageLimit& limit) const;

    void ThrowIfLimitExceeded(const std::string& model_name, const UsageLimit& limit) const;

private:
    void CheckLimit(const TotalUsage& usage, const UsageLimit& limit) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, TotalUsage> usage_by_model_;
};

}// namespace flock
