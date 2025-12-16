#include "flock/registry/registry.hpp"
#include "flock/functions/aggregate/llm_rerank.hpp"

namespace flock {

void AggregateRegistry::RegisterLlmRerank(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::AggregateFunction(
            "llm_rerank", {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
            duckdb::LogicalType::JSON(), duckdb::AggregateFunction::StateSize<AggregateFunctionState>,
            LlmRerank::Initialize, LlmRerank::Operation, LlmRerank::Combine, LlmRerank::Finalize, LlmRerank::SimpleUpdate,
            LlmRerank::Bind, LlmRerank::Destroy));
}

}// namespace flock
