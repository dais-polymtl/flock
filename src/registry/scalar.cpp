#include "flockmtl/registry/scalar.hpp"

namespace flockmtl {

void ScalarRegistry::Register(duckdb::DatabaseInstance& db) {
    RegisterLlmCompleteJson(db);
    RegisterLlmComplete(db);
    RegisterLlmEmbedding(db);
    RegisterLlmFilter(db);
    RegisterFusionRRF(db);
    RegisterFusionCombMNZ(db);
    RegisterFusionCombSUM(db);
}

} // namespace flockmtl
