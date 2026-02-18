#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/llm_filter.hpp"

namespace flock {

void ScalarRegistry::RegisterLlmFilter(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("llm_filter",
                                                   {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                   duckdb::LogicalType::VARCHAR, LlmFilter::Execute,
                                                   LlmFilter::Bind));
}

}// namespace flock
