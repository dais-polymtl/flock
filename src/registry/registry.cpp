#include "flock/registry/registry.hpp"

namespace flock {

void Registry::Register(duckdb::ExtensionLoader& loader) {
    RegisterAggregateFunctions(loader);
    RegisterScalarFunctions(loader);
}

void Registry::RegisterAggregateFunctions(duckdb::ExtensionLoader& loader) { AggregateRegistry::Register(loader); }

void Registry::RegisterScalarFunctions(duckdb::ExtensionLoader& loader) { ScalarRegistry::Register(loader); }

}// namespace flock
