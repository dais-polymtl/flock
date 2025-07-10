#include "flockmtl/registry/aggregate.hpp"

namespace flockmtl {

void AggregateRegistry::Register(duckdb::DatabaseInstance& db) {
    RegisterLlmFirst(db);
    RegisterLlmLast(db);
    RegisterLlmRerank(db);
    RegisterLlmReduce(db);
}

}// namespace flockmtl
