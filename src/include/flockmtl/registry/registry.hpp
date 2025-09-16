#pragma once

#include "flockmtl/core/common.hpp"
#include "flockmtl/registry/aggregate.hpp"
#include "flockmtl/registry/scalar.hpp"

namespace flockmtl {

class Registry {
public:
    static void Register(duckdb::ExtensionLoader& loader);

private:
    static void RegisterAggregateFunctions(duckdb::ExtensionLoader& loader);
    static void RegisterScalarFunctions(duckdb::ExtensionLoader& loader);
};

}// namespace flockmtl
