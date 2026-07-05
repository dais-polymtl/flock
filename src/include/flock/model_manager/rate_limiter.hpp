#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace flock {

class ModelRateLimiter {
public:
    ModelRateLimiter(const std::chrono::steady_clock::duration window = std::chrono::seconds(60)) : window_(window) {}

    // Blocks until admitting `request_count` more requests keeps the number of
    // requests in the trailing window at or below
    // `rate_limit` for the given model. Bursts up to `rate_limit` are allowed
    // without waiting.
    void WaitForBatchIfNeeded(const std::string& model_name, size_t request_count, size_t rate_limit);

    void Reset();

private:
    std::mutex mutex_;
    std::chrono::steady_clock::duration window_;
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> request_times_;
};

}// namespace flock
