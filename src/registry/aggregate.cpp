#include "flockmtl/registry/aggregate.hpp"

namespace flockmtl {

void AggregateRegistry::Register(duckdb::ExtensionLoader& loader) {
    RegisterLlmFirst(loader);
    RegisterLlmLast(loader);
    RegisterLlmRerank(loader);
    RegisterLlmReduce(loader);
}

}// namespace flockmtl
