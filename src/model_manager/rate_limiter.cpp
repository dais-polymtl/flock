#include "flock/model_manager/rate_limiter.hpp"

#include <algorithm>
#include <thread>

namespace flock {

void ModelRateLimiter::WaitForBatchIfNeeded(size_t request_count, size_t rate_limit) {
    if (request_count == 0 || rate_limit == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    while (request_count > 0) {
        const auto now = std::chrono::steady_clock::now();
        const auto cutoff = now - window_;
        while (!request_times_.empty() && request_times_.front() <= cutoff) {
            request_times_.pop_front();
        }

        const auto capacity = rate_limit - request_times_.size();
        if (capacity > 0) {
            const auto admit = std::min(capacity, request_count);
            request_times_.insert(request_times_.end(), static_cast<size_t>(admit), now);
            request_count -= admit;
            if (request_count == 0) {
                return;
            }
        }

        // Window is full: wait until the oldest in-window request ages out.
        std::this_thread::sleep_until(request_times_.front() + window_);
    }
}

void ModelRateLimiter::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    request_times_.clear();
}

}// namespace flock
