#pragma once

#include "flock/core/common.hpp"

namespace flock {

class ScalarRegistry {
public:
    static void Register(duckdb::ExtensionLoader& loader);

private:
    static void RegisterLlmComplete(duckdb::ExtensionLoader& loader);
    static void RegisterLlmEmbedding(duckdb::ExtensionLoader& loader);
    static void RegisterLlmFilter(duckdb::ExtensionLoader& loader);
    static void RegisterFusionRRF(duckdb::ExtensionLoader& loader);
    static void RegisterFusionCombANZ(duckdb::ExtensionLoader& loader);
    static void RegisterFusionCombMED(duckdb::ExtensionLoader& loader);
    static void RegisterFusionCombMNZ(duckdb::ExtensionLoader& loader);
    static void RegisterFusionCombSUM(duckdb::ExtensionLoader& loader);
    static void RegisterFlockGetMetrics(duckdb::ExtensionLoader& loader);
    static void RegisterFlockGetDebugMetrics(duckdb::ExtensionLoader& loader);
    static void RegisterFlockResetMetrics(duckdb::ExtensionLoader& loader);
};

}// namespace flock
