#include "flockmtl-test/functions/scalar/test_fusion.hpp"
#include <flockmtl/functions/scalar/fusion_combanz.hpp>

#include <iostream>

using namespace duckdb;

TEST_CASE("Unit test for flockmtl::FusionCombANZ with 2 DOUBLES", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 1 (one row)
    chunk.Initialize(allocator, types, 1);

    // Set the cardinality (number of rows) to 1
    chunk.SetCardinality(1);

    // Populate the DataChunk with test data
    chunk.SetValue(0, 0, 1.0);
    chunk.SetValue(1, 0, 2.0);

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // Verify the result, it should start with "-1"
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].compare(0, 2, "-1") == 0);
}

TEST_CASE("Unit test for flockmtl::FusionCombANZ with multiple rows", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 1 (one row)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 1
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk
    constexpr std::array<double, 5> bm25_scores = {10.0, 20.0, 30.0, 40.0, 50.0};
    constexpr std::array<double, 5> vs_scores = {30.0, 10.0, 2.0, 200.0, 15.0};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < bm25_scores.size(); ++i) {
        chunk.SetValue(0, i, bm25_scores[i]);
        chunk.SetValue(1, i, vs_scores[i]);
    }

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // Verify the result
    constexpr std::array<char, 5> expected_results = {'5', '4', '3', '1', '2'};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i][0] == expected_results[i]);
    }
}

TEST_CASE("Unit test for flockmtl::FusionCombANZ with some NULL values", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 1 (one row)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 1
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk. -1 means null
    constexpr std::array<double, 5> bm25_scores = {-1, 20.0, -1, 40.0, 50.0};
    constexpr std::array<double, 5> vs_scores = {30.0, 10.0, 2.0, 200.0, -1};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < bm25_scores.size(); ++i) {
        if (bm25_scores[i] == -1) {
            chunk.SetValue(0, i, duckdb::Value(duckdb::LogicalType::DOUBLE));
        } else {
            chunk.SetValue(0, i, bm25_scores[i]);
        }

        if (vs_scores[i] == -1) {
            chunk.SetValue(1, i, duckdb::Value(duckdb::LogicalType::DOUBLE));
        } else {
            chunk.SetValue(1, i, vs_scores[i]);
        }
    }

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // Verify the result
    constexpr std::array<char, 5> expected_results = {'4', '3', '5', '2', '1'};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i][0] == expected_results[i]);
    }
}

TEST_CASE("Unit test for flockmtl::FusionCombANZ with entire NULL column", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk
    constexpr std::array<double, 5> vs_scores = {21.046906565486452, 20.43271756518423, 21.75739696774139, 20.520761702528056, 21.58813126808257};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < vs_scores.size(); ++i) {
        chunk.SetValue(0, i, duckdb::Value(duckdb::LogicalType::DOUBLE));   // This creates NULL values
        chunk.SetValue(1, i, vs_scores[i]);
    }

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // Verify the result
    constexpr std::array<char, 5> expected_results = {'3', '5', '1', '4', '2'};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i][0] == expected_results[i]);
    }
}

/** This test case is only here for stability reasons and to make sure the function doesn't crash.
 *  We won't check the return value, because the behaviour is undefined. It makes no sense to rank only NULL values.
 */
TEST_CASE("Unit test for flockmtl::FusionCombANZ with only NULL values", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Populate the DataChunk with test data
    for (size_t i = 0; i < 5; ++i) {
        chunk.SetValue(0, i, duckdb::Value(duckdb::LogicalType::DOUBLE));   // This creates NULL values
        chunk.SetValue(1, i, duckdb::Value(duckdb::LogicalType::DOUBLE));   // This creates NULL values
    }

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // The results aren't checked because the behaviour is undefined. Ranking only NULL values doesn't make sense.
    // We're just making sure the function didn't throw any runtime errors.
}

TEST_CASE("Unit test for flockmtl::FusionCombANZ with empty DataChunk", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 1 (one row). The allocator doesn't allow us to have 0 rows.
    chunk.Initialize(allocator, types, 1);

    // Set the cardinality (number of rows) to 1
    chunk.SetCardinality(1);

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // Verify the result
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].compare(0, 2, "-1") == 0);
}

TEST_CASE("Unit test for flockmtl::FusionCombANZ with extreme values", "[fusion_combanz][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 1 (one row)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 1
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk
    constexpr double max_double = std::numeric_limits<double>::max();
    constexpr double min_double = std::numeric_limits<double>::lowest();
    constexpr std::array<double, 5> bm25_scores = {max_double, max_double * 0.95, max_double / 2, max_double * 0.25, 0.0};
    constexpr std::array<double, 5> vs_scores = {0.0, min_double * 0.25, min_double / 2, min_double * 0.95, min_double};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < bm25_scores.size(); ++i) {
        chunk.SetValue(0, i, bm25_scores[i]);
        chunk.SetValue(1, i, vs_scores[i]);
    }

    // Call FusionCombANZ with the prepared DataChunk
    const std::vector<string> result = flockmtl::FusionCombANZ::Operation(chunk);

    // Verify the result
    constexpr std::array<char, 5> expected_results = {'1', '2', '3', '4', '5'};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i][0] == expected_results[i]);
    }
}