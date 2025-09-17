#pragma once

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>

#include "flock/core/config.hpp"
#include "flock/prompt_manager/repository.hpp"

namespace flock {

class PromptManager {
public:
    static std::string ReplaceSection(const std::string& prompt_template, const PromptSection section,
                                      const std::string& section_content);
    static std::string ReplaceSection(const std::string& prompt_template, const std::string& replace_string,
                                      const std::string& section_content);

    template<typename T>
    static std::string ToString(const T element);
    template<typename T>
    static T FromString(const std::string& element);

    template<typename FunctionType>
    static std::string GetTemplate(FunctionType option) {
        auto prompt_template =
                PromptManager::ReplaceSection(META_PROMPT, PromptSection::INSTRUCTIONS, INSTRUCTIONS::Get(option));
        auto response_format = RESPONSE_FORMAT::Get(option);
        prompt_template =
                PromptManager::ReplaceSection(prompt_template, PromptSection::RESPONSE_FORMAT, response_format);
        return prompt_template;
    };

    static PromptDetails CreatePromptDetails(const nlohmann::json& prompt_details_json);

    static std::string ConstructNumTuples(int num_tuples);

    static std::string ConstructInputTuplesHeader(const nlohmann::json& columns, const std::string& tuple_format = "XML");
    static std::string ConstructInputTuplesHeaderXML(const nlohmann::json& columns);
    static std::string ConstructInputTuplesHeaderMarkdown(const nlohmann::json& columns);

    static std::string ConstructInputTuplesXML(const nlohmann::json& columns);
    static std::string ConstructInputTuplesMarkdown(const nlohmann::json& columns);
    static std::string ConstructInputTuplesJSON(const nlohmann::json& columns);

    static std::string ConstructInputTuples(const nlohmann::json& columns, const std::string& tuple_format = "XML");

    template<typename FunctionType>
    static std::tuple<std::string, nlohmann::json> Render(const std::string& user_prompt, const nlohmann::json& columns, FunctionType option,
                                                          const std::string& tuple_format = "XML") {
        auto media_data = nlohmann::json::array();
        auto tabular_data = nlohmann::json::array();
        for (auto i = 0; i < static_cast<int>(columns.size()); i++) {
            if (columns[i].contains("type") && columns[i]["type"] == "image") {
                media_data.push_back(columns[i]);
            } else {
                tabular_data.push_back(columns[i]);
            }
        }

        auto prompt = PromptManager::GetTemplate(option);
        prompt = PromptManager::ReplaceSection(prompt, PromptSection::USER_PROMPT, user_prompt);
        if (!tabular_data.empty()) {
            auto tuples = PromptManager::ConstructInputTuples(tabular_data, tuple_format);
            prompt = PromptManager::ReplaceSection(prompt, PromptSection::TUPLES, tuples);
        }
        return {prompt, media_data};
    };
};

}// namespace flock
