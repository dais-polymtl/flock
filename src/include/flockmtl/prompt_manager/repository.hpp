#pragma once

#include <string>
#include <unordered_map>

namespace flockmtl {

enum class PromptSection { USER_PROMPT,
                           TUPLES,
                           RESPONSE_FORMAT,
                           INSTRUCTIONS };

enum class AggregateFunctionType { REDUCE,
                                   FIRST,
                                   LAST,
                                   RERANK };

enum class ScalarFunctionType { COMPLETE,
                                FILTER };

enum class TupleFormat { XML,
                         JSON,
                         Markdown };

inline std::unordered_map<std::string, TupleFormat> TUPLE_FORMAT = {
        {"XML", TupleFormat::XML},
        {"JSON", TupleFormat::JSON},
        {"MARKDOWN", TupleFormat::Markdown}};

TupleFormat stringToTupleFormat(const std::string& format);

constexpr auto META_PROMPT =
        "You are FlockMTL a semantic analysis tool for DBMS. You will analyze each tuple in the provided data and respond "
        "to "
        "the user prompt.\n\nUser Prompt:\n\n- {{USER_PROMPT}}\n\nTuples "
        "Table:\n\n{{TUPLES}}\n\nInstructions:\n\n{{INSTRUCTIONS}}\n\nExpected Response Format:\n\n{{RESPONSE_FORMAT}}";

class INSTRUCTIONS {
public:
    static constexpr auto SCALAR_FUNCTION =
            "- The response should be directly relevant to each tuple without additional formatting, purely answering the "
            "user prompt as if each tuple were a standalone entity.\n- Use clear, context-relevant language to generate a "
            "meaningful and concise answer for each tuple.";
    static constexpr auto AGGREGATE_FUNCTION =
            "- For each tuple in the provided data, evaluate the relevant attribute(s) based on the user prompt.\n- After "
            "evaluating each tuple, aggregate the results according to the user's query. This could involve different "
            "operations depending on the prompt.\n- The aggregation should summarize the responses of all tuples into a "
            "single final result that answers the user's question as a whole.\n- The final result should be returned "
            "according to the expected response format.";

    template<typename FunctionType>
    static std::string Get(FunctionType option);
};

class RESPONSE_FORMAT {
public:
    // Scalar Functions
    static constexpr auto COMPLETE =
            "You should return the responses to the user's prompt for each tuple in plain text. Ensure no tuple is "
            "missed.";
    static constexpr auto FILTER =
            "You should return the responses to the user's prompt for each tuple in a "
            "BOOL format that would be true/false.";

    // Aggregate Functions
    static constexpr auto REDUCE =
            "Return a single, coherent output that synthesizes the most relevant information from the tuples provided.";
    static constexpr auto FIRST_OR_LAST =
            "Identify the {{RELEVANCE}} relevant tuple based on the provided user prompt from the list of tuples. "
            "Output the index of this single tuple";
    static constexpr auto RERANK =
            "Rank the tuples in descending order of relevance according to the user prompt. Return the index of each tuple in the order of their relevance.";

    template<typename FunctionType>
    static std::string Get(const FunctionType option);
};

struct PromptDetails {
    std::string prompt_name;
    std::string prompt;
    int version = -1;
};

}// namespace flockmtl
