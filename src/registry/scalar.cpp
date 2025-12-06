#include "flock/registry/scalar.hpp"

namespace flock {

void ScalarRegistry::Register(duckdb::ExtensionLoader& loader) {
    RegisterLlmComplete(loader);
    RegisterLlmEmbedding(loader);
    RegisterLlmFilter(loader);
    RegisterFusionRRF(loader);
    RegisterFusionCombANZ(loader);
    RegisterFusionCombMED(loader);
    RegisterFusionCombMNZ(loader);
    RegisterFusionCombSUM(loader);
    RegisterFlockGetMetrics(loader);
    RegisterFlockGetDebugMetrics(loader);
    RegisterFlockResetMetrics(loader);
}

}// namespace flock
