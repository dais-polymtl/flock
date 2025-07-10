#include "flockmtl/registry/scalar.hpp"

namespace flockmtl {

void ScalarRegistry::Register(duckdb::DatabaseInstance& db) {
    RegisterLlmComplete(db);
    RegisterLlmEmbedding(db);
    RegisterLlmFilter(db);
    RegisterFusionRRF(db);
    RegisterFusionCombANZ(db);
    RegisterFusionCombMED(db);
    RegisterFusionCombMNZ(db);
    RegisterFusionCombSUM(db);
}

}// namespace flockmtl
