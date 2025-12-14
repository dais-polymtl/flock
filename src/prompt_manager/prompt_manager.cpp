#include "flock/prompt_manager/prompt_manager.hpp"

namespace flock {
template<>
std::string PromptManager::ToString<PromptSection>(const PromptSection section) {
    switch (section) {
        case PromptSection::USER_PROMPT:
            return "{{USER_PROMPT}}";
        case PromptSection::TUPLES:
            return "{{TUPLES}}";
        case PromptSection::RESPONSE_FORMAT:
            return "{{RESPONSE_FORMAT}}";
        case PromptSection::INSTRUCTIONS:
            return "{{INSTRUCTIONS}}";
        default:
            return "";
    }
}

std::string PromptManager::ReplaceSection(const std::string& prompt_template, const PromptSection section,
                                          const std::string& section_content) {
    auto replace_string = PromptManager::ToString(section);
    return PromptManager::ReplaceSection(prompt_template, replace_string, section_content);
}

std::string PromptManager::ReplaceSection(const std::string& prompt_template, const std::string& replace_string,
                                          const std::string& section_content) {
    auto prompt = prompt_template;
    auto replace_string_size = replace_string.size();
    auto replace_pos = prompt.find(replace_string);

    while (replace_pos != std::string::npos) {
        prompt.replace(replace_pos, replace_string_size, section_content);
        replace_pos = prompt.find(replace_string, replace_pos + section_content.size());
    }

    return prompt;
}

std::string PromptManager::ConstructInputTuplesHeader(const nlohmann::json& columns,
                                                      const std::string& tuple_format) {
    switch (stringToTupleFormat(tuple_format)) {
        case TupleFormat::XML:
            return ConstructInputTuplesHeaderXML(columns);
        case TupleFormat::Markdown:
            return ConstructInputTuplesHeaderMarkdown(columns);
        case TupleFormat::JSON:
            return "";
        default:
            throw std::runtime_error("Invalid tuple format provided `" + tuple_format + "`");
    }
}

std::string PromptManager::ConstructInputTuplesHeaderXML(const nlohmann::json& columns) {
    if (columns.empty()) {
        return "<header></header>\n";
    }
    auto header = std::string("<header>");
    auto column_idx = 1u;
    for (const auto& column: columns) {
        std::string column_name;
        if (column.contains("name") && column["name"].is_string()) {
            column_name = column["name"].get<std::string>();
        } else {
            column_name = "COLUMN " + std::to_string(column_idx++);
        }
        header += "<column>" + column_name + "</column>";
    }
    header += "</header>\n";
    return header;
}

std::string PromptManager::ConstructInputTuplesHeaderMarkdown(const nlohmann::json& columns) {
    if (columns.empty()) {
        return " | Empty | \n | ----- | \n";
    }
    auto header = std::string(" | ");
    auto column_idx = 1u;
    for (const auto& column: columns) {
        if (column.contains("name") && column["name"].is_string()) {
            header += "COLUMN_" + column["name"].get<std::string>() + " | ";
        } else {
            header += "COLUMN " + std::to_string(column_idx++) + " | ";
        }
    }
    header += "\n | ";
    column_idx = 1u;
    for (const auto& column: columns) {
        std::string column_name;
        if (column.contains("name") && column["name"].is_string()) {
            column_name = column["name"].get<std::string>();
        } else {
            column_name = "COLUMN " + std::to_string(column_idx++);
        }
        header += std::string(column_name.length(), '-') + " | ";
    }
    header += "\n";
    return header;
}

std::string PromptManager::ConstructInputTuplesXML(const nlohmann::json& columns) {
    if (columns.empty() || columns[0]["data"].empty()) {
        return "<row></row>\n";
    }

    auto tuples_str = std::string("");
    for (auto i = 0; i < static_cast<int>(columns[0]["data"].size()); i++) {
        tuples_str += "<row>";
        for (const auto& column: columns) {
            std::string value_str;
            const auto& data_item = column["data"][i];
            if (data_item.is_null()) {
                value_str = "";
            } else if (data_item.is_string()) {
                value_str = data_item.get<std::string>();
            } else {
                value_str = data_item.dump();
            }
            tuples_str += "<column>" + value_str + "</column>";
        }
        tuples_str += "</row>\n";
    }
    return tuples_str;
}

std::string PromptManager::ConstructInputTuplesMarkdown(const nlohmann::json& columns) {
    if (columns.empty() || columns[0]["data"].empty()) {
        return "";
    }

    auto tuples_str = std::string("");
    for (auto i = 0; i < static_cast<int>(columns[0]["data"].size()); i++) {
        tuples_str += " | ";
        for (const auto& column: columns) {
            tuples_str += column["data"][i].dump() + " | ";
        }
        tuples_str += "\n";
    }
    return tuples_str;
}

std::string PromptManager::ConstructInputTuplesJSON(const nlohmann::json& columns) {
    auto tuples_json = nlohmann::json::object();
    auto column_idx = 1u;
    for (const auto& column: columns) {
        std::string column_name;
        if (column.contains("name") && column["name"].is_string()) {
            column_name = column["name"].get<std::string>();
        } else {
            column_name = "COLUMN " + std::to_string(column_idx++);
        }
        tuples_json[column_name] = column["data"];
    }
    auto tuples_str = tuples_json.dump(4);
    tuples_str += "\n";
    return tuples_str;
}

std::string PromptManager::ConstructNumTuples(const int num_tuples) {
    return "- The Number of Tuples to Generate Responses for: " + std::to_string(num_tuples) + "\n\n";
}

std::string PromptManager::ConstructInputTuples(const nlohmann::json& columns, const std::string& tuple_format) {
    auto tuples_str = std::string("");
    const auto num_tuples = columns.size() > 0 ? static_cast<int>(columns[0]["data"].size()) : 0;

    tuples_str += PromptManager::ConstructNumTuples(num_tuples);
    tuples_str += PromptManager::ConstructInputTuplesHeader(columns, tuple_format);
    switch (const auto format = stringToTupleFormat(tuple_format)) {
        case TupleFormat::XML:
            return tuples_str + ConstructInputTuplesXML(columns);
        case TupleFormat::Markdown:
            return tuples_str + ConstructInputTuplesMarkdown(columns);
        case TupleFormat::JSON:
            return tuples_str + ConstructInputTuplesJSON(columns);
        default:
            throw std::runtime_error("Invalid tuple format provided `" + tuple_format + "`");
    }
}

PromptDetails PromptManager::CreatePromptDetails(const nlohmann::json& prompt_details_json) {
    PromptDetails prompt_details;

    try {
        if (prompt_details_json.contains("prompt_name")) {
            if (!prompt_details_json.contains("version") && prompt_details_json.size() > 1) {
                throw std::runtime_error("");
            } else if (prompt_details_json.contains("version") && prompt_details_json.size() > 2) {
                throw std::runtime_error("");
            }
            prompt_details.prompt_name = prompt_details_json["prompt_name"];
            std::string error_message;
            std::string version_where_clause;
            std::string order_by_clause;
            if (prompt_details_json.contains("version")) {
                prompt_details.version = std::stoi(prompt_details_json["version"].get<std::string>());
                version_where_clause = duckdb_fmt::format(" AND version = {}", prompt_details.version);
                error_message = duckdb_fmt::format("with version {} not found", prompt_details.version);
            } else {
                order_by_clause = " ORDER BY version DESC LIMIT 1 ";
                error_message += "not found";
            }
            const auto prompt_details_query =
                    duckdb_fmt::format(" SELECT prompt, version "
                                       "   FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "  WHERE prompt_name = '{}'"
                                       " {} "
                                       " UNION ALL "
                                       " SELECT prompt, version "
                                       "   FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "  WHERE prompt_name = '{}'"
                                       " {} {}",
                                       prompt_details.prompt_name, version_where_clause, prompt_details.prompt_name,
                                       version_where_clause, order_by_clause);
            error_message = duckdb_fmt::format("The provided `{}` prompt " + error_message, prompt_details.prompt_name);
            auto con = Config::GetConnection();
            Config::StorageAttachmentGuard guard(con, true);
            const auto query_result = con.Query(prompt_details_query);
            if (query_result->RowCount() == 0) {
                throw std::runtime_error(error_message);
            }
            prompt_details.prompt = query_result->GetValue(0, 0).ToString();
            prompt_details.version = query_result->GetValue(1, 0).GetValue<int32_t>();
        } else if (prompt_details_json.contains("prompt")) {
            if (prompt_details_json.size() > 1) {
                throw std::runtime_error("");
            }
            if (prompt_details_json["prompt"].get<std::string>().empty()) {
                throw std::runtime_error("The prompt cannot be empty");
            }
            prompt_details.prompt = prompt_details_json["prompt"];
        } else {
            throw std::runtime_error("");
        }
    } catch (const std::exception& e) {
        if (e.what() == std::string("")) {
            throw std::runtime_error("The prompt details struct should contain a single key value pair of prompt or "
                                     "prompt_name with prompt version");
        }
        throw std::runtime_error(e.what());
    }
    return prompt_details;
}

nlohmann::json PromptManager::TranscribeAudioColumn(const nlohmann::json& audio_column) {
    auto transcription_model_name = audio_column["transcription_model"].get<std::string>();

    // Look up the transcription model
    nlohmann::json transcription_model_json;
    transcription_model_json["model_name"] = transcription_model_name;
    Model transcription_model(transcription_model_json);

    // Add transcription requests to batch
    transcription_model.AddTranscriptionRequest(audio_column["data"]);

    // Collect transcriptions
    auto transcription_results = transcription_model.CollectTranscriptions();

    // Convert vector<nlohmann::json> to nlohmann::json array
    nlohmann::json transcriptions = nlohmann::json::array();
    for (const auto& result: transcription_results) {
        transcriptions.push_back(result);
    }

    // Create transcription column with proper naming
    auto transcription_column = nlohmann::json::object();
    std::string original_name;
    if (audio_column.contains("name") && audio_column["name"].is_string()) {
        original_name = audio_column["name"].get<std::string>();
    }
    auto transcription_name = original_name.empty() ? "transcription" : "transcription_of_" + original_name;
    transcription_column["name"] = transcription_name;
    transcription_column["data"] = transcriptions;

    return transcription_column;
}

}// namespace flock
