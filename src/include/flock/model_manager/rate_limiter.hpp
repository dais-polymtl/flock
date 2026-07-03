#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace flock {

class ModelRateLimiter {
public:
    ModelRateLimiter() = default;

    // Blocks until admitting `request_count` more requests keeps the number of
    // requests in the trailing window (60 seconds by default) at or below
    // `rate_limit` for the given model. Bursts up to `rate_limit` are allowed
    // without waiting.
    void WaitForBatch(const std::string& model_name, int request_count, int rate_limit);

    void Reset();

    // Test-only: shrink the rolling window so throttling can be observed quickly.
    // Production always uses the 60-second default.
    void SetWindowForTesting(std::chrono::steady_clock::duration window);

private:
    std::mutex mutex_;
    std::chrono::steady_clock::duration window_ = std::chrono::seconds(60);
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> request_times_;
};

}// namespace flock
