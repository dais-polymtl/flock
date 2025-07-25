#include "flockmtl/model_manager/providers/adapters/openai.hpp"

namespace flockmtl {

void OpenAIProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, bool json_response, OutputType output_type) {
    nlohmann::json request_payload = {{"model", model_details_.model},
                                      {"messages", {{{"role", "user"}, {"content", prompt}}}}};

    if (!model_details_.model_parameters.empty()) {
        request_payload.update(model_details_.model_parameters);
    }

    if (json_response) {
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
    }

    model_handler_->AddRequest(request_payload);
}

void OpenAIProvider::AddEmbeddingRequest(const std::vector<std::string>& inputs) {
    nlohmann::json request_payload = {
            {"model", model_details_.model},
            {"input", inputs},
    };

    model_handler_->AddRequest(request_payload, IModelProviderHandler::RequestType::Embedding);
}

}// namespace flockmtl
