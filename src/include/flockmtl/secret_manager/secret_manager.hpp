#pragma once

#include "flockmtl/core/common.hpp"
#include <string>

namespace flockmtl {

class SecretDetails {
public:
    std::string type;
    std::string provider;
    std::string scope;
    std::vector<std::string> fields;
    std::unordered_set<std::string> redact_fields;
    std::vector<std::string> required_fields;
};

SecretDetails get_openai_secret_details();
SecretDetails get_azure_secret_details();
SecretDetails get_ollama_secret_details();

std::vector<SecretDetails> get_secret_details_list();

class SecretManager {
public:
    enum SupportedProviders { OPENAI,
                              AZURE,
                              OLLAMA };
    static std::unordered_map<std::string, SupportedProviders> providerNames;

    static void Register(duckdb::DatabaseInstance& instance);
    static std::unordered_map<std::string, std::string> GetSecret(const std::string& secret_name);
    static SupportedProviders GetProviderType(const std::string& provider);
    static void ValidateRequiredFields(const duckdb::CreateSecretInput& input,
                                       const std::vector<std::string>& required_fields);

private:
    static void RegisterSecretType(duckdb::DatabaseInstance& instance);

    static void RegisterSecretFunction(duckdb::DatabaseInstance& instance);

    static duckdb::unique_ptr<duckdb::BaseSecret> CreateSecret(duckdb::ClientContext& context,
                                                               duckdb::CreateSecretInput& input);

    static duckdb::unique_ptr<duckdb::KeyValueSecret> ConstructBaseSecret(duckdb::vector<std::string>& prefix_paths_p,
                                                                          const SecretDetails& details,
                                                                          std::string& type, std::string& provider,
                                                                          std::string& name);
};

}// namespace flockmtl
