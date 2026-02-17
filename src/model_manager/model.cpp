#include "flock/model_manager/model.hpp"
#include "flock/secret_manager/secret_manager.hpp"

namespace flock {

// Regular expression to match a valid Base64 string
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
    auto query_result = GetQueriedModel(model_details_.model_name);
    model_details_.model =
            model_json.contains("model") ? model_json.at("model").get<std::string>() : std::get<0>(query_result);
    model_details_.provider_name =
            model_json.contains("provider") ? model_json.at("provider").get<std::string>() : std::get<1>(query_result);
    auto secret_name = "__default_" + model_details_.provider_name;
    if (model_details_.provider_name == AZURE)
        secret_name += "_llm";
    if (model_json.contains("secret_name")) {
        secret_name = model_json["secret_name"].get<std::string>();
    }
    model_details_.secret = SecretManager::GetSecret(secret_name);
    model_details_.model_parameters = model_json.contains("model_parameters") ? nlohmann::json::parse(model_json.at("model_parameters").get<std::string>()) : std::get<2>(query_result)["model_parameters"];

    model_details_.tuple_format =
            model_json.contains("tuple_format") ? model_json.at("tuple_format").get<std::string>() : std::get<2>(query_result).contains("tuple_format") ? std::get<2>(query_result).at("tuple_format").get<std::string>()
                                                                                                                                                        : "XML";

    model_details_.batch_size = model_json.contains("batch_size") ? model_json.at("batch_size").get<int>() : std::get<2>(query_result).contains("batch_size") ? std::get<2>(query_result).at("batch_size").get<int>()
                                                                                                                                                              : 2048;
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
        default:
            throw std::invalid_argument(duckdb_fmt::format("Unsupported provider: {}", model_details_.provider_name));
    }
}

ModelDetails Model::GetModelDetails() { return model_details_; }

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
