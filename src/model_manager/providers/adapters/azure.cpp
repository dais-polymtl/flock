#include "flockmtl/model_manager/providers/adapters/azure.hpp"

namespace flockmtl {

void AzureProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) {

    auto message_content = nlohmann::json::array();

    message_content.push_back({{"type", "text"}, {"text", prompt}});

    if (!media_data.empty()) {
        auto detail = media_data[0].contains("detail") ? media_data[0]["detail"].get<std::string>() : "low";
        auto image_type = media_data[0]["type"].get<std::string>();
        auto mime_type = std::string("image/");
        if (size_t pos = image_type.find("/"); pos != std::string::npos) {
            mime_type += image_type.substr(pos + 1);
        } else {
            mime_type += std::string("png");
        }
        auto column_index = 1u;
        for (const auto& column: media_data) {
            message_content.push_back(
                    {{"type", "text"},
                     {"text", "ATTACHMENT COLUMN"}});
            auto row_index = 1u;
            for (const auto& image: column["data"]) {
                message_content.push_back(
                        {{"type", "text"}, {"text", "ROW " + std::to_string(row_index) + " :"}});
                auto image_url = std::string();
                auto image_str = image.get<std::string>();
                if (is_base64(image_str)) {
                    image_url = duckdb_fmt::format("data:{};base64,{}", mime_type, image_str);
                } else {
                    image_url = image_str;
                }

                message_content.push_back(
                        {{"type", "image_url"},
                         {"image_url", {{"url", image_url}, {"detail", detail}}}});
                row_index++;
            }
            column_index++;
        }
    }

    nlohmann::json request_payload = {{"model", model_details_.model},
                                      {"messages", {{{"role", "user"}, {"content", message_content}}}}};

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