#pragma once

#include "flock/core/common.hpp"

namespace flock {

class AggregateRegistry {
public:
    static void Register(duckdb::ExtensionLoader& loader);

private:
    static void RegisterLlmFirst(duckdb::ExtensionLoader& loader);
    static void RegisterLlmLast(duckdb::ExtensionLoader& loader);
    static void RegisterLlmRerank(duckdb::ExtensionLoader& loader);
    static void RegisterLlmReduce(duckdb::ExtensionLoader& loader);
};

}// namespace flock
