#include "flockmtl/registry/registry.hpp"

namespace flockmtl {

void Registry::Register(duckdb::ExtensionLoader& loader) {
    RegisterAggregateFunctions(loader);
    RegisterScalarFunctions(loader);
}

void Registry::RegisterAggregateFunctions(duckdb::ExtensionLoader& loader) { AggregateRegistry::Register(loader); }

void Registry::RegisterScalarFunctions(duckdb::ExtensionLoader& loader) { ScalarRegistry::Register(loader); }

}// namespace flockmtl
