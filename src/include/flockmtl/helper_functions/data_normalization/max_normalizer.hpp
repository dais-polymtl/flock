#pragma once
#include "flockmtl/helper_functions/data_normalization/normalizer.hpp"

namespace flockmtl {
/**
 * Concrete normalizer that implements max normalization strategy.
 */
class MaxNormalizer : public Normalizer {
public:
    /**
     * Concrete method to max-normalize a vector of scores.
     * @param scores Vector of scores to normalize (modified in-place)
     * @param hit_counts Vector to track hit counts (only increments provided hit counts, modified in-place)
     * @return void
     */
    void normalize(std::vector<double>& scores, std::vector<int>& hit_counts) const override;

    /**
     * Static convenience method that creates a temporary MaxNormalizer and calls its normalize method.
     * @param scores Vector of scores to normalize (modified in-place)
     * @param hit_counts Vector to track hit counts (only increments provided hit counts, modified in-place)
     * @return void
     */
    static void normalizeStatic(std::vector<double>& scores, std::vector<int>& hit_counts);
};

} // namespace flockmtl