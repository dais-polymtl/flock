#include "flock/registry/aggregate.hpp"

namespace flock {

void AggregateRegistry::Register(duckdb::ExtensionLoader& loader) {
    RegisterLlmFirst(loader);
    RegisterLlmLast(loader);
    RegisterLlmRerank(loader);
    RegisterLlmReduce(loader);
}

}// namespace flock
