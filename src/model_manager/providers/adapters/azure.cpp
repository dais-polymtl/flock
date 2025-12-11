#include "flock/model_manager/providers/adapters/azure.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/model_manager/providers/handlers/url_handler.hpp"

namespace flock {

void AzureProvider::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) {

    auto message_content = nlohmann::json::array();

    message_content.push_back({{"type", "text"}, {"text", prompt}});

    // Process image columns
    if (media_data.contains("image") && !media_data["image"].empty() && media_data["image"].is_array()) {
        std::string detail = "low";
        auto column_index = 1u;
        for (const auto& column: media_data["image"]) {
            // Process image column as before
            if (column_index == 1) {
                detail = column.contains("detail") ? column["detail"].get<std::string>() : "low";
            }
            auto image_type = column.contains("type") ? column["type"].get<std::string>() : "image";
            auto mime_type = std::string("image/");
            if (size_t pos = image_type.find("/"); pos != std::string::npos) {
                mime_type += image_type.substr(pos + 1);
            } else {
                mime_type += std::string("png");
            }
            message_content.push_back(
                    {{"type", "text"},
                     {"text", "ATTACHMENT COLUMN"}});
            auto row_index = 1u;
            for (const auto& image: column["data"]) {
                // Skip null values
                if (image.is_null()) {
                    continue;
                }
                message_content.push_back(
                        {{"type", "text"}, {"text", "ROW " + std::to_string(row_index) + " :"}});
                auto image_url = std::string();
                std::string image_str;
                if (image.is_string()) {
                    image_str = image.get<std::string>();
                } else {
                    image_str = image.dump();
                }

                // Handle file path or URL
                if (URLHandler::IsUrl(image_str)) {
                    // URL - send directly to API
                    image_url = image_str;
                } else {
                    // File path - read and convert to base64
                    auto base64_result = URLHandler::ResolveFileToBase64(image_str);
                    image_url = duckdb_fmt::format("data:{};base64,{}", mime_type, base64_result.base64_content);
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
                 {{"name", "flock_response"},
                  {"strict", strict},
                  {"schema", {{"type", "object"}, {"properties", {{"items", {{"type", "array"}, {"minItems", num_output_tuples}, {"maxItems", num_output_tuples}, {"items", schema}}}}}, {"required", {"items"}}, {"additionalProperties", false}}}}}};
    } else {
        request_payload["response_format"] = {
                {"type", "json_schema"},
                {"json_schema",
                 {{"name", "flock_response"},
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

void AzureProvider::AddTranscriptionRequest(const nlohmann::json& audio_files) {
    for (const auto& audio_file: audio_files) {
        auto audio_file_str = audio_file.get<std::string>();

        // Handle file download and validation
        auto file_result = URLHandler::ResolveFilePath(audio_file_str);

        nlohmann::json transcription_request = {
                {"file_path", file_result.file_path},
                {"model", model_details_.model},
                {"is_temp_file", file_result.is_temp_file}};
        model_handler_->AddRequest(transcription_request, IModelProviderHandler::RequestType::Transcription);
    }
}

}// namespace flock