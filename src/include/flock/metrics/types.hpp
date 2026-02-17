#pragma once

#include <cstddef>
#include <cstdint>

namespace flock {

enum class FunctionType : uint8_t {
    LLM_COMPLETE = 0,
    LLM_FILTER = 1,
    LLM_EMBEDDING = 2,
    LLM_REDUCE = 3,
    LLM_RERANK = 4,
    LLM_FIRST = 5,
    LLM_LAST = 6,
    UNKNOWN = 7
};

inline constexpr const char* FunctionTypeToString(FunctionType type) noexcept {
    switch (type) {
        case FunctionType::LLM_COMPLETE:
            return "llm_complete";
        case FunctionType::LLM_FILTER:
            return "llm_filter";
        case FunctionType::LLM_EMBEDDING:
            return "llm_embedding";
        case FunctionType::LLM_REDUCE:
            return "llm_reduce";
        case FunctionType::LLM_RERANK:
            return "llm_rerank";
        case FunctionType::LLM_FIRST:
            return "llm_first";
        case FunctionType::LLM_LAST:
            return "llm_last";
        default:
            return "unknown";
    }
}

inline constexpr size_t FunctionTypeToIndex(FunctionType type) noexcept {
    return static_cast<size_t>(type);
}

}// namespace flock
