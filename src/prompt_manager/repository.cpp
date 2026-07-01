#include "flock/prompt_manager/repository.hpp"
#include "duckdb/common/assert.hpp"
#include "flock/prompt_manager/prompt_manager.hpp"
#include <algorithm>

namespace flock {

template<>
std::string INSTRUCTIONS::Get(ScalarFunctionType option) {
    return INSTRUCTIONS::SCALAR_FUNCTION;
};

template<>
std::string INSTRUCTIONS::Get(AggregateFunctionType option) {
    return INSTRUCTIONS::AGGREGATE_FUNCTION;
};

template<>
std::string RESPONSE_FORMAT::Get(const ScalarFunctionType option) {
    switch (option) {
        case ScalarFunctionType::COMPLETE:
            return RESPONSE_FORMAT::COMPLETE;
        case ScalarFunctionType::FILTER:
            return RESPONSE_FORMAT::FILTER;
        default:
            return "";
    }
}

template<>
std::string RESPONSE_FORMAT::Get(const AggregateFunctionType option) {
    switch (option) {
        case AggregateFunctionType::REDUCE:
            return RESPONSE_FORMAT::REDUCE;
        case AggregateFunctionType::RERANK:
            return RESPONSE_FORMAT::RERANK;
        case AggregateFunctionType::FIRST:
        case AggregateFunctionType::LAST: {
            return PromptManager::ReplaceSection(RESPONSE_FORMAT::FIRST_OR_LAST, "{{RELEVANCE}}",
                                                 option == AggregateFunctionType::FIRST ? "most" : "least");
        }
        default:
            return "";
    }
}

TupleFormat stringToTupleFormat(const std::string& format) {
    auto upper_format = format;
    std::transform(upper_format.begin(), upper_format.end(), upper_format.begin(), ::toupper);
    if (TUPLE_FORMAT.find(upper_format) != TUPLE_FORMAT.end()) {
        return TUPLE_FORMAT.at(upper_format);
    }
    throw std::runtime_error("Expected 'tuple_format' to be one of: JSON, XML, or Markdown.");
}

TupleFormat tupleFormatFromStoredValue(const nlohmann::json& value) {
    if (!value.is_number_integer()) {
        throw std::runtime_error("Expected 'tuple_format' to be an integer.");
    }
    switch (static_cast<TupleFormat>(value.get<int>())) {
        case TupleFormat::XML:
        case TupleFormat::JSON:
        case TupleFormat::Markdown:
            return static_cast<TupleFormat>(value.get<int>());
    }
    throw std::runtime_error("Expected 'tuple_format' to be one of: JSON, XML, or Markdown.");
}

std::string tupleFormatToString(const TupleFormat format) {
    switch (format) {
        case TupleFormat::XML:
            return "XML";
        case TupleFormat::JSON:
            return "JSON";
        case TupleFormat::Markdown:
            return "Markdown";
    }
}

}// namespace flock
