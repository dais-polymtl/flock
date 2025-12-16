#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/llm_embedding.hpp"

namespace flock {

void ScalarRegistry::RegisterLlmEmbedding(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(
            duckdb::ScalarFunction("llm_embedding", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                   duckdb::LogicalType::LIST(duckdb::LogicalType::DOUBLE),
                                   LlmEmbedding::Execute, LlmEmbedding::Bind));
}

}// namespace flock
