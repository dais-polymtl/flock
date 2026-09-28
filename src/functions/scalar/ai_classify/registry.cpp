#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/ai_classify.hpp"

namespace flock {

void ScalarRegistry::RegisterAiClassify(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("ai_classify",
                                                   {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                   duckdb::LogicalType::STRUCT({{"choice", duckdb::LogicalType::VARCHAR},
                                                                                {"confidence", duckdb::LogicalType::DOUBLE},
                                                                                {"probabilities", duckdb::LogicalType::MAP(duckdb::LogicalType::VARCHAR, duckdb::LogicalType::DOUBLE)}}),
                                                   AiClassify::Execute,
                                                   AiClassify::Bind));
}

}// namespace flock
