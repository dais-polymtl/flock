#pragma once

#include <fmt/format.h>

#include "flock/core/common.hpp"
#include "flock/core/config.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/prompt_manager/repository.hpp"
#include <nlohmann/json.hpp>

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

    // Helper function to transcribe audio column and create transcription text column
    static nlohmann::json TranscribeAudioColumn(const nlohmann::json& audio_column);

public:
    template<typename FunctionType>
    static std::tuple<std::string, nlohmann::json> Render(const std::string& user_prompt, const nlohmann::json& columns, FunctionType option,
                                                          const std::string& tuple_format = "XML") {
        auto image_data = nlohmann::json::array();
        auto tabular_data = nlohmann::json::array();

        for (auto i = 0; i < static_cast<int>(columns.size()); i++) {
            if (columns[i].contains("type")) {
                auto column_type = columns[i]["type"].get<std::string>();
                if (column_type == "image") {
                    image_data.push_back(columns[i]);
                } else if (column_type == "audio") {
                    // Transcribe audio and merge as tabular text data
                    if (columns[i].contains("transcription_model")) {
                        auto transcription_column = TranscribeAudioColumn(columns[i]);
                        tabular_data.push_back(transcription_column);
                    }
                } else {
                    tabular_data.push_back(columns[i]);
                }
            } else {
                tabular_data.push_back(columns[i]);
            }
        }

        // Create media_data as an object with only image array (audio is now in tabular_data)
        nlohmann::json media_data;
        media_data["image"] = image_data;
        media_data["audio"] = nlohmann::json::array();// Empty - audio is now in tabular_data

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
