#include "flockmtl/model_manager/providers/adapters/azure.hpp"

namespace flockmtl {

void AzureProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type) {
    // Create a JSON request payload with the provided parameters
    nlohmann::json request_payload = {{"messages", {{{"role", "user"}, {"content", prompt}}}}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    if (model_details_.model_parameters.contains("response_format")) {
        auto schema = model_details_.model_parameters["response_format"]["json_schema"]["schema"];
        auto strict = model_details_.model_parameters["response_format"]["strict"];
        request_payload["response_format"] = {
                {"type", "json_schema"},
                {"json_schema",
                 {{"name", "flockmtl_response"},
                  {"strict", strict},
                  {"schema", {{"type", "object"}, {"properties", {{"items", {{"type", "array"}, {"minItems", num_output_tuples}, {"maxItems", num_output_tuples}, {"items", schema}}}}}, {"required", {"items"}}, {"additionalProperties", false}}}}}};
    } else {
        request_payload["response_format"] = {
                {"type", "json_schema"},
                {"json_schema",
                 {{"name", "flockmtl_response"},
                  {"strict", false},
                  {"schema", {{"type", "object"}, {"properties", {{"items", {{"type", "array"}, {"minItems", num_output_tuples}, {"maxItems", num_output_tuples}, {"items", {{"type", GetOutputTypeString(output_type)}}}}}}}}}}}};
        ;
    }

    model_handler_->AddRequest(request_payload, IModelProviderHandler::RequestType::Completion);
}

void AzureProvider::AddEmbeddingRequest(const std::vector<std::string>& inputs) {
    for (const auto& input: inputs) {
        nlohmann::json request_payload = {
                {"model", model_details_.model},
                {"prompt", input},
        };

        model_handler_->AddRequest(request_payload, IModelProviderHandler::RequestType::Embedding);
    }
}

}// namespace flockmtl