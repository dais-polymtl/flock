#include "flockmtl/registry/registry.hpp"
#include "flockmtl/functions/scalar/llm_complete.hpp"

namespace flockmtl {

void ScalarRegistry::RegisterLlmComplete(duckdb::DatabaseInstance& db) {
    duckdb::ExtensionUtil::RegisterFunction(db, duckdb::ScalarFunction("llm_complete", {}, duckdb::LogicalType::JSON(),
                                                                       LlmComplete::Execute, nullptr, nullptr, nullptr,
                                                                       nullptr, duckdb::LogicalType::ANY));
}

}// namespace flockmtl
