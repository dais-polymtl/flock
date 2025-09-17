#include "flock/core/config.hpp"
#include <gtest/gtest.h>

namespace flock {

class GlobalTestEnvironment : public ::testing::Environment {
public:
    std::unique_ptr<duckdb::DuckDB> db;

    void SetUp() override {
        db = std::make_unique<duckdb::DuckDB>("unit_test.db");
        Config::GetConnection(&*db->instance);
    }
};

}// namespace flock

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new flock::GlobalTestEnvironment());
    return RUN_ALL_TESTS();
}
