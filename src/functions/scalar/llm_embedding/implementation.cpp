#include "flockmtl/functions/scalar/llm_embedding.hpp"

namespace flockmtl {

void LlmEmbedding::ValidateArguments(duckdb::DataChunk& args) {
    if (args.ColumnCount() < 2 || args.ColumnCount() > 2) {
        throw std::runtime_error("LlmEmbedScalarParser: Invalid number of arguments.");
    }
    if (args.data[0].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("LlmEmbedScalarParser: Model details must be a struct.");
    }
    if (args.data[1].GetType().id() != duckdb::LogicalTypeId::STRUCT) {
        throw std::runtime_error("LlmEmbedScalarParser: Inputs must be a struct.");
    }
}

std::vector<duckdb::vector<duckdb::Value>> LlmEmbedding::Operation(duckdb::DataChunk& args) {
    LlmEmbedding::ValidateArguments(args);

    auto inputs = CastVectorOfStructsToJson(args.data[1], args.size());
    auto model_details_json = CastVectorOfStructsToJson(args.data[0], 1)[0];
    Model model(model_details_json);

    std::vector<std::string> prepared_inputs;
    for (auto& row : inputs) {
        std::string concat_input;
        for (auto& item : row.items()) {
            concat_input += item.value().get<std::string>() + " ";
        }
        prepared_inputs.push_back(concat_input);
    }

    auto batch_size = model.GetModelDetails().batch_size;

    if (batch_size == 0 || batch_size > prepared_inputs.size()) {
        batch_size = static_cast<int>(prepared_inputs.size());
    }

    std::vector<duckdb::vector<duckdb::Value>> results;
    for (size_t i = 0; i < prepared_inputs.size(); i += batch_size) {
        std::vector<std::string> batch_inputs;
        for (size_t j = i; j < i + batch_size && j < prepared_inputs.size(); j++) {
            batch_inputs.push_back(prepared_inputs[j]);
        }
        auto embeddings = model.CallEmbedding(batch_inputs);
        for (size_t index = 0; index < embeddings.size(); index++) {
            duckdb::vector<duckdb::Value> embedding;
            for (auto& value : embeddings[index]) {
                embedding.push_back(duckdb::Value(static_cast<double>(value)));
            }
            results.push_back(embedding);
        }
    }
    return results;
}

void LlmEmbedding::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = LlmEmbedding::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value::LIST(res));
    }
}

} // namespace flockmtl
