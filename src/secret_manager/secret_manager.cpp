#include "flock/secret_manager/secret_manager.hpp"
#include "flock/core/config.hpp"
#include <unordered_map>

#include <duckdb/main/secret/secret_manager.hpp>

namespace flock {

SecretDetails get_openai_secret_details() {
    return {"openai", "flock", "openai://", {"base_url", "api_key"}, {"api_key"}, {"api_key"}};
}

SecretDetails get_azure_secret_details() {
    return {"azure_llm", "flock", "azure_llm://", {"api_key", "resource_name", "api_version"}, {"api_key"}, {"api_key", "resource_name", "api_version"}};
}

SecretDetails get_ollama_secret_details() {
    return {"ollama", "flock", "ollama://", {"api_url"}, {"api_url"}, {"api_url"}};
}

SecretDetails get_anthropic_secret_details() {
    return {"anthropic", "flock", "anthropic://", {"api_key", "api_version"}, {"api_key"}, {"api_key"}};
}

std::vector<SecretDetails> get_secret_details_list() {
    return {get_openai_secret_details(), get_azure_secret_details(), get_ollama_secret_details(), get_anthropic_secret_details()};
}

std::unordered_map<std::string, SecretManager::SupportedProviders> SecretManager::providerNames = {
        {"openai", OPENAI},
        {"azure_llm", AZURE},
        {"ollama", OLLAMA},
        {"anthropic", ANTHROPIC}};

SecretManager::SupportedProviders SecretManager::GetProviderType(const std::string& provider) {
    auto it = providerNames.find(provider);
    if (it == providerNames.end()) {
        throw duckdb::InvalidInputException("Unsupported secret provider: %s", provider.c_str());
    }
    return it->second;
}

void SecretManager::Register(duckdb::ExtensionLoader& loader) {
    RegisterSecretType(loader);
    RegisterSecretFunction(loader);
}

void SecretManager::RegisterSecretType(duckdb::ExtensionLoader& loader) {
    for (const auto& secret_detail: get_secret_details_list()) {
        duckdb::SecretType secret_type;
        secret_type.name = secret_detail.type;
        secret_type.deserializer = duckdb::KeyValueSecret::Deserialize<duckdb::KeyValueSecret>;
        secret_type.default_provider = secret_detail.provider;

        loader.RegisterSecretType(secret_type);
    }
}

void SecretManager::RegisterSecretFunction(duckdb::ExtensionLoader& loader) {
    for (const auto& secret_details: get_secret_details_list()) {
        duckdb::CreateSecretFunction secret_function = {secret_details.type, secret_details.provider, CreateSecret};

        for (const auto& field: secret_details.fields) {
            secret_function.named_parameters[field] = duckdb::LogicalType::VARCHAR;
        }

        loader.RegisterFunction(secret_function);
    }
}

duckdb::unique_ptr<duckdb::BaseSecret> SecretManager::CreateSecret(duckdb::ClientContext& context,
                                                                   duckdb::CreateSecretInput& input) {
    SecretDetails selected_details;

    if (GetProviderType(input.type) == SupportedProviders::OPENAI) {
        selected_details = get_openai_secret_details();
    } else if (GetProviderType(input.type) == SupportedProviders::AZURE) {
        selected_details = get_azure_secret_details();
    } else if (GetProviderType(input.type) == SupportedProviders::OLLAMA) {
        selected_details = get_ollama_secret_details();
    } else if (GetProviderType(input.type) == SupportedProviders::ANTHROPIC) {
        selected_details = get_anthropic_secret_details();
    } else {
        throw duckdb::InvalidInputException("Unsupported secret type: %s", input.type.c_str());
    }

    ValidateRequiredFields(input, selected_details.required_fields);

    auto prefix_paths = input.scope;
    if (prefix_paths.empty()) {
        prefix_paths.push_back(selected_details.scope);
    }

    auto secret = ConstructBaseSecret(prefix_paths, selected_details, input.type, input.provider, input.name);

    for (const auto& field: selected_details.fields) {
        if (auto lookup = input.options.find(field); lookup != input.options.end()) {
            secret->secret_map[field] = lookup->second;
        }
    }

    return std::move(secret);
}

duckdb::unique_ptr<duckdb::KeyValueSecret>
SecretManager::ConstructBaseSecret(duckdb::vector<std::string>& prefix_paths_p, const SecretDetails& details,
                                   std::string& type, std::string& provider, std::string& name) {
    auto secret = duckdb::make_uniq<duckdb::KeyValueSecret>(prefix_paths_p, type, provider, name);
    duckdb::case_insensitive_set_t redact_fields_set(details.redact_fields.begin(), details.redact_fields.end());
    secret->redact_keys = std::move(redact_fields_set);
    return secret;
}

std::unordered_map<std::string, std::string> SecretManager::GetSecret(const std::string& secret_name) {
    std::unordered_map<std::string, std::string> secret_map;
    auto& instance = Config::db;
    auto secret_manager = &instance->GetSecretManager();
    auto transaction = duckdb::CatalogTransaction::GetSystemTransaction(*instance);
    auto secret = secret_manager->GetSecretByName(transaction, secret_name);
    secret = secret_manager->GetSecretByName(transaction, secret_name);
    if (secret == nullptr) {
        throw duckdb::InvalidInputException("Secret not found : %s", secret_name.c_str());
    }
    const auto kv_secret = dynamic_cast<const duckdb::KeyValueSecret&>(*secret->secret);

    auto provider = secret->secret->GetType();
    auto providerType = SecretManager::GetProviderType(provider);
    SecretDetails secret_details;
    switch (providerType) {
        case SupportedProviders::OPENAI:
            secret_details = get_openai_secret_details();
            break;
        case SupportedProviders::AZURE:
            secret_details = get_azure_secret_details();
            break;
        case SupportedProviders::OLLAMA:
            secret_details = get_ollama_secret_details();
            break;
        case SupportedProviders::ANTHROPIC:
            secret_details = get_anthropic_secret_details();
            break;
        default:
            throw duckdb::InvalidInputException("Unsupported secret type: %s", provider.c_str());
    }

    for (const auto& field: secret_details.fields) {
        if (auto value = kv_secret.TryGetValue(field); !value.IsNull()) {
            secret_map[field] = value.ToString();
        }
    }

    return secret_map;
}

void SecretManager::ValidateRequiredFields(const duckdb::CreateSecretInput& input,
                                           const std::vector<std::string>& required_fields) {
    for (const auto& field: required_fields) {
        if (auto lookup = input.options.find(field);
            lookup == input.options.end() || lookup->second.ToString().empty()) {
            throw duckdb::InvalidInputException("Missing required field: '%s'", field.c_str());
        }
    }
}

}// namespace flock
