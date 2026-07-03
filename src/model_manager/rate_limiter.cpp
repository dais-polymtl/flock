#include "flock/model_manager/rate_limiter.hpp"

#include <algorithm>
#include <thread>

namespace flock {

void ModelRateLimiter::WaitForBatch(const std::string& model_name, int request_count, int rate_limit) {
    if (model_name.empty() || request_count <= 0 || rate_limit <= 0) {
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

        const int capacity = rate_limit - static_cast<int>(times.size());
        if (capacity > 0) {
            const int admit = std::min(capacity, request_count);
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
    window_ = std::chrono::seconds(60);
}

void ModelRateLimiter::SetWindowForTesting(std::chrono::steady_clock::duration window) {
    std::lock_guard<std::mutex> lock(mutex_);
    window_ = window;
}

}// namespace flock
