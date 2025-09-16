#include "flockmtl/registry/scalar.hpp"

namespace flockmtl {

void ScalarRegistry::Register(duckdb::ExtensionLoader& loader) {
    RegisterLlmComplete(loader);
    RegisterLlmEmbedding(loader);
    RegisterLlmFilter(loader);
    RegisterFusionRRF(loader);
    RegisterFusionCombANZ(loader);
    RegisterFusionCombMED(loader);
    RegisterFusionCombMNZ(loader);
    RegisterFusionCombSUM(loader);
}

}// namespace flockmtl
