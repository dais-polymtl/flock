#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/llm_complete.hpp"

namespace flock {

void ScalarRegistry::RegisterLlmComplete(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("llm_complete",
                                                   {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                   duckdb::LogicalType::JSON(), LlmComplete::Execute,
                                                   LlmComplete::Bind));
}

}// namespace flock
