#pragma once

#include "flockmtl/core/common.hpp"

namespace flockmtl {

class ScalarRegistry {
public:
    static void Register(duckdb::DatabaseInstance& db);

private:
    static void RegisterLlmCompleteJson(duckdb::DatabaseInstance& db);
    static void RegisterLlmComplete(duckdb::DatabaseInstance& db);
    static void RegisterLlmEmbedding(duckdb::DatabaseInstance& db);
    static void RegisterLlmFilter(duckdb::DatabaseInstance& db);
    static void RegisterFusionRRF(duckdb::DatabaseInstance& db);
    static void RegisterFusionCombANZ(duckdb::DatabaseInstance& db);
    static void RegisterFusionCombMED(duckdb::DatabaseInstance& db);
    static void RegisterFusionCombMNZ(duckdb::DatabaseInstance& db);
    static void RegisterFusionCombSUM(duckdb::DatabaseInstance& db);
};

} // namespace flockmtl
