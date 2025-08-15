#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/llm_embedding.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterLlmEmbedding(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(
            db,
            duckdb::ScalarFunction("llm_embedding", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY}, duckdb::LogicalType::LIST(duckdb::LogicalType::DOUBLE),
                                   LlmEmbedding::Execute));
}

}// namespace flockmtl
