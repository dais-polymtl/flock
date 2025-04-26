#include "flockmtl/prompt_manager/prompt_manager.hpp"

namespace flockmtl {
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

std::string PromptManager::ConstructInputTuplesHeader(const nlohmann::json& tuples,
                                                      const std::string& tuple_format) {
    switch (const auto format = stringToTupleFormat(tuple_format)) {
        case TupleFormat::XML:
            return ConstructInputTuplesHeaderXML(tuples);
        case TupleFormat::Markdown:
            return ConstructInputTuplesHeaderMarkdown(tuples);
        case TupleFormat::JSON:
            return "";
        default:
            throw std::runtime_error("Invalid tuple format provided `" + tuple_format + "`");
    }
}

std::string PromptManager::ConstructInputTuplesHeaderXML(const nlohmann::json& tuples) {
    if (tuples.empty()) {
        return "<tuple>Empty</tuple>\n";
    }
    auto header = std::string("<tuple>");
    for (const auto& key: tuples[0].items()) {
        header += "<col>" + key.key() + "</col>";
    }
    header += "</tuple>\n";
    return header;
}

std::string PromptManager::ConstructInputTuplesHeaderMarkdown(const nlohmann::json& tuples) {
    if (tuples.empty()) {
        return "| Empty |\n|---|\n";
    }
    auto header = std::string("|");
    for (const auto& key: tuples[0].items()) {
        header += key.key() + "|";
    }
    header += "\n|";
    for (const auto& key: tuples[0].items()) {
        header += "---|";
    }
    header += "\n";
    return header;
}

std::string PromptManager::ConstructSingleInputTuple(const nlohmann::json& tuple, const std::string& tuple_format) {
    switch (const auto format = stringToTupleFormat(tuple_format)) {
        case TupleFormat::XML:
            return ConstructSingleInputTupleXML(tuple);
        case TupleFormat::Markdown:
            return ConstructSingleInputTupleMarkdown(tuple);
        case TupleFormat::JSON:
            return ConstructSingleInputTupleJSON(tuple);
        default:
            throw std::runtime_error("Invalid tuple format provided `" + tuple_format + "`");
    }
}

std::string PromptManager::ConstructSingleInputTupleXML(const nlohmann::json& tuple) {
    auto tuple_str = std::string("<tuple>");
    for (const auto& key: tuple.items()) {
        tuple_str += "<col>" + key.value().dump() + "</col>";
    }
    tuple_str += "</tuple>\n";
    return tuple_str;
}

std::string PromptManager::ConstructSingleInputTupleMarkdown(const nlohmann::json& tuple) {
    auto tuple_str = std::string("|");
    for (const auto& key: tuple.items()) {
        tuple_str += key.value().dump() + "|";
    }
    tuple_str += "\n";
    return tuple_str;
}

std::string PromptManager::ConstructSingleInputTupleJSON(const nlohmann::json& tuple) {
    return tuple.dump() + "\n";
}

std::string PromptManager::ConstructNumTuples(const int num_tuples) {
    return "- The Number of Tuples to Generate Responses for: " + std::to_string(num_tuples) + "\n\n";
}

std::string PromptManager::ConstructInputTuples(const nlohmann::json& tuples, const std::string& tuple_format) {
    auto tuples_str = std::string("");
    tuples_str += PromptManager::ConstructNumTuples(static_cast<int>(tuples.size()));
    tuples_str += PromptManager::ConstructInputTuplesHeader(tuples, tuple_format);
    for (const auto& tuple: tuples) {
        tuples_str += PromptManager::ConstructSingleInputTuple(tuple, tuple_format);
    }
    return tuples_str;
}

PromptDetails PromptManager::CreatePromptDetails(const nlohmann::json& prompt_details_json) {
    PromptDetails prompt_details;

    if (prompt_details_json.contains("prompt_name")) {
        if (!prompt_details_json.contains("version") && prompt_details_json.size() > 1) {
            throw std::runtime_error(
                    "The prompt details struct should contain a single key value pair of prompt or "
                    "prompt_name with prompt version");
        } else if (prompt_details_json.contains("version") && prompt_details_json.size() > 2) {
            throw std::runtime_error(
                    "The prompt details struct should contain a single key value pair of prompt or "
                    "prompt_name with prompt version");
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
                                   "   FROM flockmtl_storage.flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                   "  WHERE prompt_name = '{}'"
                                   " {} "
                                   " UNION ALL "
                                   " SELECT prompt, version "
                                   "   FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                   "  WHERE prompt_name = '{}'"
                                   " {} {}",
                                   prompt_details.prompt_name, version_where_clause, prompt_details.prompt_name,
                                   version_where_clause, order_by_clause);
        error_message = duckdb_fmt::format("The provided `{}` prompt " + error_message, prompt_details.prompt_name);
        auto con = Config::GetConnection();
        const auto query_result = con.Query(prompt_details_query);
        if (query_result->RowCount() == 0) {
            throw std::runtime_error(error_message);
        }
        prompt_details.prompt = query_result->GetValue(0, 0).ToString();
    } else if (prompt_details_json.contains("prompt")) {
        prompt_details.prompt = prompt_details_json["prompt"];
    } else {
        throw std::runtime_error("The prompt details struct should contain a single key value pair of prompt or "
                                 "prompt_name with prompt version");
    }
    return prompt_details;
}
}// namespace flockmtl
