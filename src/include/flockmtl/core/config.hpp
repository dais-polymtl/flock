#pragma once

#include "filesystem.hpp"
#include "flockmtl/core/common.hpp"
#include "flockmtl/registry/registry.hpp"
#include <fmt/format.h>

namespace flockmtl {

enum ConfigType { LOCAL,
                  GLOBAL };

class Config {
public:
    static duckdb::DatabaseInstance* db;
    static duckdb::DatabaseInstance* global_db;
    static duckdb::Connection GetConnection(duckdb::DatabaseInstance* db = nullptr);
    static duckdb::Connection GetGlobalConnection();
    static void Configure(duckdb::DatabaseInstance& db);
    static void ConfigureGlobal();
    static void ConfigureTables(duckdb::Connection& con, ConfigType type);
    static void ConfigureLocal(duckdb::DatabaseInstance& db);

    static std::string get_schema_name();
    static std::filesystem::path get_global_storage_path();
    static std::string get_default_models_table_name();
    static std::string get_user_defined_models_table_name();
    static std::string get_prompts_table_name();
    constexpr static int32_t default_context_window = 128000;
    constexpr static int32_t default_max_output_tokens = 4096;

private:
    static void SetupGlobalStorageLocation();
    static void ConfigSchema(duckdb::Connection& con, std::string& schema_name);
    static void ConfigPromptTable(duckdb::Connection& con, std::string& schema_name, ConfigType type);
    static void ConfigModelTable(duckdb::Connection& con, std::string& schema_name, ConfigType type);
    static void SetupDefaultModelsConfig(duckdb::Connection& con, std::string& schema_name);
    static void SetupUserDefinedModelsConfig(duckdb::Connection& con, std::string& schema_name);
};

}// namespace flockmtl
