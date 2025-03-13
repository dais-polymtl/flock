#pragma once
#include "flockmtl/core/common.hpp"

namespace flockmtl {

/**
 * Abstract Normalizer class that defines the interface for vector normalization strategies.
 */
class Normalizer {
public:
    virtual ~Normalizer() = default;
    /**
     * Virtual normalize method for polymorphic behavior.
     * @param scores Vector of scores to normalize (modified in-place)
     * @param hit_counts Vector which tracks hit counts (only increments provided hit counts, modified in-place)
     * @return Nothing, the vector is modified in place
     */
    virtual void normalize(std::vector<double>& scores, std::vector<int>& hit_counts) const = 0;
};

} // namespace flockmtl