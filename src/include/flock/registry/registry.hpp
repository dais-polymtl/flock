#pragma once

#include "flock/core/common.hpp"
#include "flock/registry/aggregate.hpp"
#include "flock/registry/scalar.hpp"

namespace flock {

class Registry {
public:
    static void Register(duckdb::ExtensionLoader& loader);

private:
    static void RegisterAggregateFunctions(duckdb::ExtensionLoader& loader);
    static void RegisterScalarFunctions(duckdb::ExtensionLoader& loader);
};

}// namespace flock
