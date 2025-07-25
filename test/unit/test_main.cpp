#include "flockmtl/core/config.hpp"
#include <gtest/gtest.h>

namespace flockmtl {

class GlobalTestEnvironment : public ::testing::Environment {
public:
    std::unique_ptr<duckdb::DuckDB> db;

    void SetUp() override {
        db = std::make_unique<duckdb::DuckDB>("flockmtl_test.db");
        Config::GetConnection(&*db->instance);
    }
};

}// namespace flockmtl

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new flockmtl::GlobalTestEnvironment());
    return RUN_ALL_TESTS();
}
