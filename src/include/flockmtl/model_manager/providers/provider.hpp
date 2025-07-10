#pragma once

#include "fmt/format.h"
#include <nlohmann/json.hpp>

#include "flockmtl/model_manager/repository.hpp"

namespace flockmtl {

enum class OutputType {
    STRING,
    OBJECT,
    BOOL,
    INTEGER
};

class IProvider {
public:
    ModelDetails model_details_;

    explicit IProvider(const ModelDetails& model_details) : model_details_(model_details) {};
    virtual ~IProvider() = default;

    virtual nlohmann::json CallComplete(const std::string& prompt, bool json_response, OutputType output_type) = 0;
    virtual nlohmann::json CallEmbedding(const std::vector<std::string>& inputs) = 0;

    static std::string GetOutputTypeString(const OutputType output_type) {
        switch (output_type) {
            case OutputType::STRING:
                return "string";
            case OutputType::OBJECT:
                return "object";
            case OutputType::BOOL:
                return "boolean";
            case OutputType::INTEGER:
                return "integer";
            default:
                throw std::invalid_argument("Unsupported output type");
        }
    }
};

class ExceededMaxOutputTokensError : public std::exception {
public:
    const char* what() const noexcept override {
        return "The response exceeded the max_output_tokens length; increase your max_output_tokens parameter.";
    }
};

}// namespace flockmtl
