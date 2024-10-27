#pragma once
#include "flockmtl/common.hpp"

namespace flockmtl {

namespace core {

struct CoreModule {
public:
    static Connection &GetConnection(DatabaseInstance *db = nullptr);
    static void Register(DatabaseInstance &db);
    static void EnsureTableExists(duckdb::Connection &con);
};

} // namespace core

} // namespace flockmtl
