#include "flock/functions/scalar/scalar.hpp"
#include "flock/model_manager/model.hpp"
#include <algorithm>
#include <cstddef>
#include <duckdb/planner/expression/bound_function_expression.hpp>
#include <vector>

namespace flock {

namespace {

nlohmann::json BuildBatchTuples(const nlohmann::json& tuples, int start_index, int batch_size) {
    auto batch_tuples = nlohmann::json::array();

    for (auto i = 0; i < static_cast<int>(tuples.size()); i++) {
        batch_tuples.push_back(nlohmann::json::object());
        for (const auto& item: tuples[i].items()) {
            if (item.key() != "data") {
                batch_tuples[i][item.key()] = item.value();
                continue;
            }

            for (auto j = 0; j < batch_size && start_index + j < static_cast<int>(item.value().size()); j++) {
                if (j == 0) {
                    batch_tuples[i]["data"] = nlohmann::json::array();
                }
                batch_tuples[i]["data"].push_back(item.value()[start_index + j]);
            }
        }
    }

    return batch_tuples;
}

void NormalizeAndAppendBatchResponse(nlohmann::json response, size_t expected_size, nlohmann::json& responses) {
    if (response.size() < expected_size) {
        for (auto i = response.size(); i < expected_size; i++) {
            response.push_back(nullptr);
        }
    } else if (response.size() > expected_size) {
        response.erase(response.begin() + static_cast<std::ptrdiff_t>(expected_size), response.end());
    }

    for (const auto& tuple: response) {
        responses.push_back(tuple);
    }
}

nlohmann::json BuildNullResponsesForRowCount(int row_count) {
    auto responses = nlohmann::json::array();

    for (auto i = 0; i < row_count; i++) {
        responses.push_back(nullptr);
    }

    return responses;
}

struct AsyncBatchWork {
    int start_index;
    int batch_size;
};

void WriteBatchResponseToResults(const nlohmann::json& batch_response,
                                 int start_index,
                                 int expected_size,
                                 nlohmann::json& responses) {
    const auto& items = batch_response["items"];
    const auto count = std::min<int>(static_cast<int>(items.size()), expected_size);
    for (int j = 0; j < count; j++) {
        responses[start_index + j] = items[j];
    }
    for (int j = count; j < expected_size; j++) {
        responses[start_index + j] = nullptr;
    }
}

void NullBatchRows(int start_index, int row_count, int rows_to_null, nlohmann::json& responses) {
    for (int j = 0; j < rows_to_null && start_index + j < row_count; j++) {
        responses[start_index + j] = nullptr;
    }
}

void RequeueOrNullFailedBatch(const AsyncBatchWork& work,
                              int row_count,
                              std::vector<AsyncBatchWork>& pending,
                              nlohmann::json& responses) {
    const int new_batch_size = work.batch_size / 2;
    if (new_batch_size <= 0) {
        NullBatchRows(work.start_index, row_count, work.batch_size, responses);
        return;
    }

    pending.push_back({work.start_index, new_batch_size});
    const int remainder = work.batch_size - new_batch_size;
    if (remainder > 0) {
        pending.push_back({work.start_index + new_batch_size, remainder});
    }
}

}// namespace

void ScalarFunctionBase::ValidateArgumentCount(
        const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
        const std::string& function_name) {
    if (arguments.size() != 2) {
        throw duckdb::BinderException(
                function_name + " requires 2 arguments: (1) model, (2) prompt with context_columns. Got " +
                std::to_string(arguments.size()));
    }
}

void ScalarFunctionBase::ValidateArgumentTypes(
        const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
        const std::string& function_name) {
    if (arguments[0]->return_type.id() != duckdb::LogicalTypeId::STRUCT) {
        throw duckdb::BinderException(function_name + ": First argument must be model (struct type)");
    }
    if (arguments[1]->return_type.id() != duckdb::LogicalTypeId::STRUCT) {
        throw duckdb::BinderException(
                function_name + ": Second argument must be prompt with context_columns (struct type)");
    }
}

ScalarFunctionBase::PromptStructInfo ScalarFunctionBase::ExtractPromptStructInfo(
        const duckdb::LogicalType& prompt_type) {
    PromptStructInfo info{false, std::nullopt, ""};

    for (idx_t i = 0; i < duckdb::StructType::GetChildCount(prompt_type); i++) {
        auto field_name = duckdb::StructType::GetChildName(prompt_type, i);
        if (field_name == "context_columns") {
            info.has_context_columns = true;
        } else if (field_name == "prompt" || field_name == "prompt_name") {
            if (!info.prompt_field_index.has_value()) {
                info.prompt_field_index = i;
                info.prompt_field_name = field_name;
            }
        }
    }

    return info;
}

void ScalarFunctionBase::ValidatePromptStructFields(const PromptStructInfo& info,
                                                    const std::string& function_name,
                                                    bool require_context_columns) {
    if (require_context_columns && !info.has_context_columns) {
        throw duckdb::BinderException(
                function_name + ": Second argument must contain 'context_columns' field");
    }
}

void ScalarFunctionBase::InitializeModelJson(
        duckdb::ClientContext& context,
        const duckdb::unique_ptr<duckdb::Expression>& model_expr,
        LlmFunctionBindData& bind_data) {
    if (!model_expr->IsFoldable()) {
        return;
    }

    auto model_value = duckdb::ExpressionExecutor::EvaluateScalar(context, *model_expr);
    auto user_model_json = CastValueToJson(model_value);
    bind_data.model_json = Model::ResolveModelDetailsToJson(user_model_json);
}

void ScalarFunctionBase::QueueCompletion(nlohmann::json& tuples, const std::string& user_prompt,
                                         ScalarFunctionType function_type, Model& model) {
    const auto [prompt, media_data] = PromptManager::Render(user_prompt, tuples, function_type, model.GetModelDetails().tuple_format);
    OutputType output_type = OutputType::STRING;
    if (function_type == ScalarFunctionType::FILTER) {
        output_type = OutputType::BOOL;
    }

    model.AddCompletionRequest(prompt, static_cast<int>(tuples[0]["data"].size()), output_type, media_data);
}

nlohmann::json ScalarFunctionBase::Complete(nlohmann::json& columns, const std::string& user_prompt,
                                            ScalarFunctionType function_type, Model& model) {
    QueueCompletion(columns, user_prompt, function_type, model);
    auto response = model.CollectCompletions();
    return response[0]["items"];
};

nlohmann::json ScalarFunctionBase::BatchAndCompleteSync(const nlohmann::json& tuples,
                                                        const std::string& user_prompt,
                                                        const ScalarFunctionType function_type, Model& model) {
    const int row_count = static_cast<int>(tuples[0]["data"].size());
    const int configured = std::min<int>(model.GetModelDetails().batch_size, row_count);
    auto batch_size = configured;

    auto responses = nlohmann::json::array();

    int start_index = 0;

    do {
        auto batch_tuples = BuildBatchTuples(tuples, start_index, batch_size);

        start_index += batch_size;

        try {
            auto response = Complete(batch_tuples, user_prompt, function_type, model);
            NormalizeAndAppendBatchResponse(response, batch_tuples[0]["data"].size(), responses);
            batch_size = configured;
        } catch (const TokenLimitExceededError&) {
            start_index -= batch_size;
            const int attempted_batch_size = batch_size;
            batch_size = batch_size / 2;
            if (batch_size <= 0) {
                const int remaining = row_count - start_index;
                const int rows_to_null = std::min<int>(attempted_batch_size, remaining);
                for (int i = 0; i < rows_to_null; i++) {
                    responses.push_back(nullptr);
                }
                start_index += rows_to_null;
                batch_size = configured;
            }
        }

    } while (start_index < row_count);

    return responses;
}

nlohmann::json ScalarFunctionBase::BatchAndCompleteAsync(const nlohmann::json& tuples,
                                                         const std::string& user_prompt,
                                                         const ScalarFunctionType function_type, Model& model) {
    const int row_count = static_cast<int>(tuples[0]["data"].size());
    const int configured = std::min<int>(model.GetModelDetails().batch_size, row_count);

    auto responses = BuildNullResponsesForRowCount(row_count);
    std::vector<AsyncBatchWork> pending;

    for (int start_index = 0; start_index < row_count; start_index += configured) {
        pending.push_back({start_index, std::min<int>(configured, row_count - start_index)});
    }

    while (!pending.empty()) {
        auto attempt_model = Model(model.GetModelDetailsAsJson());
        const auto current_round = std::move(pending);
        pending.clear();

        for (const auto& work: current_round) {
            auto batch_tuples = BuildBatchTuples(tuples, work.start_index, work.batch_size);
            QueueCompletion(batch_tuples, user_prompt, function_type, attempt_model);
        }

        std::vector<nlohmann::json> batch_responses;
        bool collect_threw_token_error = false;
        try {
            batch_responses = attempt_model.CollectCompletions();
        } catch (const TokenLimitExceededError&) {
            collect_threw_token_error = true;
        }

        if (collect_threw_token_error) {
            for (const auto& work: current_round) {
                RequeueOrNullFailedBatch(work, row_count, pending, responses);
            }
            continue;
        }

        if (batch_responses.size() != current_round.size()) {
            throw std::runtime_error(
                    duckdb_fmt::format("Expected {} completion batch responses, got {}",
                                       current_round.size(), batch_responses.size()));
        }

        for (size_t i = 0; i < current_round.size(); i++) {
            const auto& work = current_round[i];
            if (IsTokenLimitExceededMarker(batch_responses[i])) {
                RequeueOrNullFailedBatch(work, row_count, pending, responses);
            } else {
                WriteBatchResponseToResults(batch_responses[i], work.start_index, work.batch_size, responses);
            }
        }
    }

    return responses;
}

nlohmann::json ScalarFunctionBase::BatchAndComplete(const nlohmann::json& tuples,
                                                    const std::string& user_prompt,
                                                    const ScalarFunctionType function_type, Model& model) {
    if (model.GetModelDetails().is_async) {
        return BatchAndCompleteAsync(tuples, user_prompt, function_type, model);
    }

    return BatchAndCompleteSync(tuples, user_prompt, function_type, model);
}

void ScalarFunctionBase::InitializePrompt(
        duckdb::ClientContext& context,
        const duckdb::unique_ptr<duckdb::Expression>& prompt_expr,
        LlmFunctionBindData& bind_data) {
    nlohmann::json prompt_json;

    if (prompt_expr->IsFoldable()) {
        auto prompt_value = duckdb::ExpressionExecutor::EvaluateScalar(context, *prompt_expr);
        prompt_json = CastValueToJson(prompt_value);
    } else if (prompt_expr->expression_class == duckdb::ExpressionClass::BOUND_FUNCTION) {
        auto& func_expr = prompt_expr->Cast<duckdb::BoundFunctionExpression>();
        const auto& struct_type = prompt_expr->return_type;

        for (idx_t i = 0; i < duckdb::StructType::GetChildCount(struct_type) && i < func_expr.children.size(); i++) {
            auto field_name = duckdb::StructType::GetChildName(struct_type, i);
            auto& child = func_expr.children[i];

            if (field_name != "context_columns" && child->IsFoldable()) {
                try {
                    auto field_value = duckdb::ExpressionExecutor::EvaluateScalar(context, *child);
                    if (field_value.type().id() == duckdb::LogicalTypeId::VARCHAR) {
                        prompt_json[field_name] = field_value.GetValue<std::string>();
                    } else {
                        prompt_json[field_name] = CastValueToJson(field_value);
                    }
                } catch (...) {
                    // Skip fields that can't be evaluated
                }
            }
        }
    }

    if (prompt_json.contains("context_columns")) {
        prompt_json.erase("context_columns");
    }

    auto prompt_details = PromptManager::CreatePromptDetails(prompt_json);
    bind_data.prompt = prompt_details.prompt;
}

duckdb::unique_ptr<LlmFunctionBindData> ScalarFunctionBase::ValidateAndInitializeBindData(
        duckdb::ClientContext& context,
        duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
        const std::string& function_name,
        bool require_context_columns,
        bool initialize_prompt) {

    ValidateArgumentCount(arguments, function_name);
    ValidateArgumentTypes(arguments, function_name);

    const auto& prompt_type = arguments[1]->return_type;
    auto prompt_info = ExtractPromptStructInfo(prompt_type);
    ValidatePromptStructFields(prompt_info, function_name, require_context_columns);

    auto bind_data = duckdb::make_uniq<LlmFunctionBindData>();

    InitializeModelJson(context, arguments[0], *bind_data);
    if (initialize_prompt) {
        InitializePrompt(context, arguments[1], *bind_data);
    }

    return bind_data;
}

}// namespace flock
