#pragma once

#include "flockmtl/core/common.hpp"

namespace flockmtl {

class AggregateRegistry {
public:
    static void Register(duckdb::DatabaseInstance& db);

private:
    static void RegisterLlmFirst(duckdb::DatabaseInstance& db);
    static void RegisterLlmLast(duckdb::DatabaseInstance& db);
    static void RegisterLlmRerank(duckdb::DatabaseInstance& db);
    static void RegisterLlmReduce(duckdb::DatabaseInstance& db);
};

}// namespace flockmtl
