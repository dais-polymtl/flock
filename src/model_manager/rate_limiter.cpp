#include "flock/model_manager/rate_limiter.hpp"

#include <algorithm>
#include <thread>

namespace flock {

void ModelRateLimiter::WaitForBatchIfNeeded(const std::string& model_name, size_t request_count, size_t rate_limit) {
    if (model_name.empty() || request_count == 0 || rate_limit == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& times = request_times_[model_name];

    while (request_count > 0) {
        const auto now = std::chrono::steady_clock::now();
        const auto cutoff = now - window_;
        while (!times.empty() && times.front() <= cutoff) {
            times.pop_front();
        }

        const auto capacity = rate_limit - times.size();
        if (capacity > 0) {
            const auto admit = std::min(capacity, request_count);
            times.insert(times.end(), static_cast<size_t>(admit), now);
            request_count -= admit;
            if (request_count == 0) {
                return;
            }
        }

        // Window is full: wait until the oldest in-window request ages out.
        std::this_thread::sleep_until(times.front() + window_);
    }
}

void ModelRateLimiter::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    request_times_.clear();
}
}// namespace flock
