#include <flockmtl/common.hpp>
#include <flockmtl/core/functions/scalar.hpp>
#include <flockmtl/core/parser/scalar.hpp>
#include <flockmtl_extension.hpp>

namespace flockmtl {
namespace core {

void CoreScalarParsers::LlmEmbeddingScalarParser(DataChunk &args) {
    if (args.ColumnCount() < 2 || args.ColumnCount() > 2) {
        throw std::runtime_error("LlmEmbedScalarParser: Invalid number of arguments.");
    }
    if (args.data[0].GetType().id() != LogicalTypeId::STRUCT) {
        throw std::runtime_error("LlmEmbedScalarParser: Inputs must be a struct.");
    }
    // check if model details is a struct
    if (args.data[1].GetType().id() != LogicalTypeId::STRUCT) {
        throw std::runtime_error("LlmEmbedScalarParser: Model details must be a struct.");
    }
}

} // namespace core
} // namespace flockmtl