#pragma once

#include "flockmtl/core/common.hpp"

namespace flockmtl {

class AggregateRegistry {
public:
    static void Register(duckdb::ExtensionLoader& loader);

private:
    static void RegisterLlmFirst(duckdb::ExtensionLoader& loader);
    static void RegisterLlmLast(duckdb::ExtensionLoader& loader);
    static void RegisterLlmRerank(duckdb::ExtensionLoader& loader);
    static void RegisterLlmReduce(duckdb::ExtensionLoader& loader);
};

}// namespace flockmtl
