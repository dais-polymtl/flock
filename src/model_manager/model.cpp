#include "flock/model_manager/model.hpp"
#include "flock/secret_manager/secret_manager.hpp"
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace flock {

const std::regex base64_regex(R"(^[A-Za-z0-9+/=]+$)");

bool is_base64(const std::string& str) {
    return std::regex_match(str, base64_regex);
}

Model::Model(const nlohmann::json& model_json) {
    LoadModelDetails(model_json);
    ConstructProvider();
}

void Model::LoadModelDetails(const nlohmann::json& model_json) {
    model_details_.model_name = model_json.contains("model_name") ? model_json.at("model_name").get<std::string>() : "";
    if (model_details_.model_name.empty()) {
        throw std::invalid_argument("`model_name` is required in model settings");
    }

    bool has_resolved_details = model_json.contains("model") &&
                                model_json.contains("provider") &&
                                model_json.contains("secret") &&
                                model_json.contains("tuple_format") &&
                                model_json.contains("batch_size");

    nlohmann::json db_model_args;

    if (has_resolved_details) {
        model_details_.model = model_json.at("model").get<std::string>();
        model_details_.provider_name = model_json.at("provider").get<std::string>();
        model_details_.secret = model_json["secret"].get<std::unordered_map<std::string, std::string>>();
        model_details_.tuple_format = model_json.at("tuple_format").get<std::string>();
        model_details_.batch_size = model_json.at("batch_size").get<int>();

        if (model_json.contains("model_parameters")) {
            auto& mp = model_json.at("model_parameters");
            model_details_.model_parameters = mp.is_string() ? nlohmann::json::parse(mp.get<std::string>()) : mp;
        } else {
            model_details_.model_parameters = nlohmann::json::object();
        }
    } else {
        auto [db_model, db_provider, db_args] = GetQueriedModel(model_details_.model_name);
        model_details_.model = model_json.contains("model") ? model_json.at("model").get<std::string>() : db_model;
        model_details_.provider_name = model_json.contains("provider") ? model_json.at("provider").get<std::string>() : db_provider;
        db_model_args = db_args;

        if (model_json.contains("secret")) {
            model_details_.secret = model_json["secret"].get<std::unordered_map<std::string, std::string>>();
        } else {
            auto secret_name = "__default_" + model_details_.provider_name;
            if (model_details_.provider_name == AZURE) {
                secret_name += "_llm";
            }
            if (model_json.contains("secret_name")) {
                secret_name = model_json["secret_name"].get<std::string>();
            }
            model_details_.secret = SecretManager::GetSecret(secret_name);
        }

        if (model_json.contains("model_parameters")) {
            auto& mp = model_json.at("model_parameters");
            model_details_.model_parameters = mp.is_string() ? nlohmann::json::parse(mp.get<std::string>()) : mp;
        } else if (db_model_args.contains("model_parameters")) {
            model_details_.model_parameters = db_model_args["model_parameters"];
        } else {
            model_details_.model_parameters = nlohmann::json::object();
        }

        if (model_json.contains("tuple_format")) {
            model_details_.tuple_format = model_json.at("tuple_format").get<std::string>();
        } else if (db_model_args.contains("tuple_format")) {
            model_details_.tuple_format = db_model_args.at("tuple_format").get<std::string>();
        } else {
            model_details_.tuple_format = "XML";
        }

        if (model_json.contains("batch_size")) {
            model_details_.batch_size = model_json.at("batch_size").get<int>();
        } else if (db_model_args.contains("batch_size")) {
            model_details_.batch_size = db_model_args.at("batch_size").get<int>();
        } else {
            model_details_.batch_size = 2048;
        }
    }
}

std::tuple<std::string, std::string, nlohmann::basic_json<>> Model::GetQueriedModel(const std::string& model_name) {
    const std::string query =
            duckdb_fmt::format(" SELECT model, provider_name, model_args "
                               " FROM flock_storage.flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                               " WHERE model_name = '{}'"
                               " UNION ALL "
                               " SELECT model, provider_name, model_args "
                               " FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                               " WHERE model_name = '{}';",
                               model_name, model_name);

    auto con = Config::GetConnection();
    Config::StorageAttachmentGuard guard(con, true);
    auto query_result = con.Query(query);

    if (query_result->RowCount() == 0) {
        query_result = con.Query(
                duckdb_fmt::format(" SELECT model, provider_name, model_args "
                                   "   FROM flock_storage.flock_config.FLOCKMTL_MODEL_DEFAULT_INTERNAL_TABLE "
                                   "  WHERE model_name = '{}' ",
                                   model_name));

        if (query_result->RowCount() == 0) {
            throw std::runtime_error("Model not found");
        }
    }

    auto model = query_result->GetValue(0, 0).ToString();
    auto provider_name = query_result->GetValue(1, 0).ToString();
    auto model_args = nlohmann::json::parse(query_result->GetValue(2, 0).ToString());

    return {model, provider_name, model_args};
}

void Model::ConstructProvider() {
    if (mock_provider_factory_) {
        provider_ = mock_provider_factory_();
        return;
    }
    if (mock_provider_) {
        provider_ = mock_provider_;
        return;
    }

    switch (GetProviderType(model_details_.provider_name)) {
        case FLOCKMTL_OPENAI:
            provider_ = std::make_shared<OpenAIProvider>(model_details_);
            break;
        case FLOCKMTL_AZURE:
            provider_ = std::make_shared<AzureProvider>(model_details_);
            break;
        case FLOCKMTL_OLLAMA:
            provider_ = std::make_shared<OllamaProvider>(model_details_);
            break;
        case FLOCKMTL_ANTHROPIC:
            provider_ = std::make_shared<AnthropicProvider>(model_details_);
            break;
        default:
            throw std::invalid_argument(duckdb_fmt::format("Unsupported provider: {}", model_details_.provider_name));
    }
}

ModelDetails Model::GetModelDetails() { return model_details_; }

nlohmann::json Model::GetModelDetailsAsJson() const {
    nlohmann::json result;
    result["model_name"] = model_details_.model_name;
    result["model"] = model_details_.model;
    result["provider"] = model_details_.provider_name;
    result["tuple_format"] = model_details_.tuple_format;
    result["batch_size"] = model_details_.batch_size;
    result["secret"] = model_details_.secret;
    if (!model_details_.model_parameters.empty()) {
        result["model_parameters"] = model_details_.model_parameters;
    }
    return result;
}

nlohmann::json Model::ResolveModelDetailsToJson(const nlohmann::json& user_model_json) {
    Model temp_model(user_model_json);
    auto resolved_json = temp_model.GetModelDetailsAsJson();

    if (user_model_json.contains("secret_name")) {
        resolved_json["secret_name"] = user_model_json["secret_name"];
    }

    return resolved_json;
}

void Model::AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type, const nlohmann::json& media_data) {
    provider_->AddCompletionRequest(prompt, num_output_tuples, output_type, media_data);
}

void Model::AddEmbeddingRequest(const std::vector<std::string>& inputs) {
    provider_->AddEmbeddingRequest(inputs);
}

void Model::AddTranscriptionRequest(const nlohmann::json& audio_files) {
    provider_->AddTranscriptionRequest(audio_files);
}

std::vector<nlohmann::json> Model::CollectCompletions(const std::string& contentType) {
    return provider_->CollectCompletions(contentType);
}

std::vector<nlohmann::json> Model::CollectEmbeddings(const std::string& contentType) {
    return provider_->CollectEmbeddings(contentType);
}

std::vector<nlohmann::json> Model::CollectTranscriptions(const std::string& contentType) {
    return provider_->CollectTranscriptions(contentType);
}

}// namespace flock
