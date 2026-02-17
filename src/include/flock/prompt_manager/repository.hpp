#pragma once

#include "flock/core/common.hpp"
#include <unordered_map>

namespace flock {

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
        "# System Setup\n"
        "You are **FlockMTL**, a semantic analysis tool for DBMS that can process both **text and image-derived data**.\n"
        "Your task is to reason over a structured dataset where **some columns originate from text and others come from external sources** like images or separate dictionaries.\n"
        "\n"
        "## Table Context\n"
        "- The section labeled **\"Table Data\"** includes all rows (rows).\n"
        "- Each row may contain standard fields, extra textual columns (converted from images or separated text), "
        "and image-related columns (e.g., image references or external attachments).\n"
        "- **Treat all these columns as part of the same table context.**\n"
        "\n"
        "## Processing Instructions\n"
        "1. Interpret the user’s prompt precisely for each row.\n"
        "2. Consider **every column**, including those derived from external content or images.\n"
        "3. If the prompt involves images, **reason about them in the context of the row’s other data**.\n"
        "\n"
        "## User’s Task\n"
        "**User Prompt**:\n"
        "```\n"
        "{{USER_PROMPT}}\n"
        "```\n"
        "\n"
        "## Table Data\n"
        "```\n"
        "{{TUPLES}}\n"
        "```\n"
        "*Some columns may be embedded as text; others may reference external images—treat them all equally.*\n"
        "\n"
        "## Instructions\n"
        "```\n"
        "{{INSTRUCTIONS}}\n"
        "```\n"
        "- Emphasize that external columns must be merged into the logical row.\n"
        "- Clarify how to balance reasoning across different column types.\n"
        "- Encourage a **step-by-step reasoning** process where appropriate.\n"
        "\n"
        "## Output Format\n"
        "```\n"
        "{{RESPONSE_FORMAT}}\n"
        "```\n"
        "Ensure your results follow this format exactly, with **no extra commentary**.\n";


class INSTRUCTIONS {
public:
    static constexpr auto SCALAR_FUNCTION =
            "- Treat each row independently as if it were a standalone record.\n"
            "- Answer the user prompt specifically for that row, without referencing other rows.\n"
            "- Do not include extra formatting or explanations—return only the relevant answer.\n"
            "- Ensure the output is concise, meaningful, and context-aware.";

    static constexpr auto AGGREGATE_FUNCTION =
            "- First, analyze each row according to the user prompt.\n"
            "- Then, aggregate the results into a single answer that addresses the prompt as a whole.\n"
            "- Aggregation may include summarizing, calculating, counting, ranking, or selecting.\n"
            "- Treat all columns (including those reconstructed from images or text with separators) as part of the table.\n"
            "- Return the aggregated answer in the expected response format, without additional commentary.";

    template<typename FunctionType>
    static std::string Get(FunctionType option);
};

class RESPONSE_FORMAT {
public:
    // Scalar Functions
    static constexpr auto COMPLETE =
            "For each row in the provided table, respond directly to the user's prompt. "
            "Ensure that each row is addressed individually and that no row is omitted. "
            "Each response should be concise, relevant, and based solely on the information within the respective row.";

    static constexpr auto FILTER =
            "For each row in the provided table, determine whether it satisfies the user's prompt. "
            "Return 'true' if the row meets the criteria, and 'false' otherwise. "
            "Ensure that each row is evaluated independently and that no row is skipped.";

    // Aggregate Functions
    static constexpr auto REDUCE =
            "Analyze each row in the provided table to extract the most pertinent information related to the user's prompt. "
            "Synthesize these individual insights into a single, coherent response that encapsulates the collective relevance of all rows.";

    static constexpr auto FIRST_OR_LAST =
            "Identify the row that is {{RELEVANCE}} relevant to the user's prompt. "
            "Return only the single index number of this row from the `flock_row_id` field, indicating its position within the provided table. "
            "The response should be a single integer value, not an array. "
            "Ensure that the relevance assessment is based solely on the information within each row.";

    static constexpr auto RERANK =
            "Evaluate the relevance of each row in the provided table concerning the user's prompt. "
            "Rank the rows in descending order of relevance and return a flat array of the row indices in this order. "
            "Use the `flock_row_id` values and return them as a simple array of integers, not nested arrays. "
            "Each row should be considered independently, and the ranking should reflect the individual pertinence of each row.";

    template<typename FunctionType>
    static std::string Get(const FunctionType option);
};

struct PromptDetails {
    std::string prompt_name;
    std::string prompt;
    int version = -1;
};

}// namespace flock
