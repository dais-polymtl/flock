#pragma once

#include "flock/core/common.hpp"
#include "flock/registry/registry.hpp"
#include <filesystem>
#include <fmt/format.h>

namespace flock {

enum ConfigType { LOCAL,
                  GLOBAL };

class Config {
public:
    static duckdb::DatabaseInstance* db;
    static duckdb::DatabaseInstance* global_db;
    static duckdb::Connection GetConnection(duckdb::DatabaseInstance* db = nullptr);
    
    static void Configure(duckdb::ExtensionLoader& loader);
    static void ConfigureGlobal(duckdb::DatabaseInstance* db_instance);
    static void ConfigureTables(duckdb::Connection& con, ConfigType type);
    static void ConfigureLocal(duckdb::DatabaseInstance& db);

    static std::string get_schema_name();
    static std::filesystem::path get_global_storage_path();
    static std::string get_default_models_table_name();
    static std::string get_user_defined_models_table_name();
    static std::string get_prompts_table_name();
    static void AttachToGlobalStorage(duckdb::Connection& con, bool read_only = true);
    static void DetachFromGlobalStorage(duckdb::Connection& con);

    class StorageAttachmentGuard {
    public:
        StorageAttachmentGuard(duckdb::Connection& con, bool read_only = true);
        ~StorageAttachmentGuard();

        StorageAttachmentGuard(const StorageAttachmentGuard&) = delete;
        StorageAttachmentGuard& operator=(const StorageAttachmentGuard&) = delete;
        StorageAttachmentGuard(StorageAttachmentGuard&&) = delete;
        StorageAttachmentGuard& operator=(StorageAttachmentGuard&&) = delete;

    private:
        duckdb::Connection& connection;
        bool attached;
        static constexpr int MAX_RETRIES = 10;
        static constexpr int RETRY_DELAY_MS = 1000;

        bool TryAttach(bool read_only);
        bool TryDetach();
        void Wait(int milliseconds);
    };

private:
    static void SetupGlobalStorageLocation(duckdb::DatabaseInstance* db_instance);
    static void ConfigSchema(duckdb::Connection& con, std::string& schema_name);
    static void ConfigPromptTable(duckdb::Connection& con, std::string& schema_name, ConfigType type);
    static void ConfigModelTable(duckdb::Connection& con, std::string& schema_name, ConfigType type);
    static void SetupDefaultModelsConfig(duckdb::Connection& con, std::string& schema_name);
    static void SetupUserDefinedModelsConfig(duckdb::Connection& con, std::string& schema_name);
};

}// namespace flock
