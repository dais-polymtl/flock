#include "flock/registry/registry.hpp"
#include "flock/functions/scalar/ai_classify.hpp"

namespace flock {

void ScalarRegistry::RegisterAiClassify(duckdb::ExtensionLoader& loader) {
    loader.RegisterFunction(duckdb::ScalarFunction("ai_classify",
                                                   {duckdb::LogicalType::ANY, duckdb::LogicalType::ANY},
                                                   duckdb::LogicalType::VARCHAR, AiClassify::Execute,
                                                   AiClassify::Bind));
}

}// namespace flock
