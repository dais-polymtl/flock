#include "flock/core/config.hpp"
#include "duckdb/common/file_system.hpp"
#include "flock/secret_manager/secret_manager.hpp"
#include <chrono>
#include <fmt/format.h>

namespace flock {

duckdb::DatabaseInstance* Config::db;

std::string Config::get_schema_name() { return "flock_config"; }

std::string Config::get_global_storage_path() {
#ifdef __EMSCRIPTEN__
    return "opfs://flock_data/flock.db";
#else
    const auto& home = duckdb::FileSystem::GetHomeDirectory(nullptr);
    if (home.empty()) {
        throw std::runtime_error("Could not find home directory");
    }
    // NOTE: DuckDB FileSystem should handle path separators
    return home + "/.duckdb/flock_storage/flock.db";
#endif
}

duckdb::Connection Config::GetConnection(duckdb::DatabaseInstance* db) {
    if (db) {
        Config::db = db;
    }
    duckdb::Connection con(*Config::db);
    return con;
}

duckdb::Connection Config::GetGlobalConnection() {
    // Meyers' Singleton"
    static duckdb::DuckDB global_db(Config::get_global_storage_path());
    duckdb::Connection con(*global_db.instance);
    return con;
}

void Config::SetupGlobalStorageLocation(duckdb::DatabaseInstance* db_instance) {
    if (!db_instance) {
        return;
    }
#ifdef __EMSCRIPTEN__
    // WASM: Client is responsible for OPFS directory/file registration
    return;
#endif
    auto& fs = duckdb::FileSystem::GetFileSystem(*db_instance);
    const std::string& global_path = get_global_storage_path();
    // Extract parent directory from path
    const auto last_slash = global_path.find_last_of('/');
    if (last_slash != std::string::npos && last_slash > 0) {
        const std::string dir_path = global_path.substr(0, last_slash);
        try {
            if (!fs.DirectoryExists(dir_path)) {
                fs.CreateDirectory(dir_path);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error creating directory " << dir_path << ": " << e.what() << std::endl;
        }
    }
}

void Config::ConfigSchema(duckdb::Connection& con, std::string& schema_name) {
    con.Query(duckdb_fmt::format("CREATE SCHEMA IF NOT EXISTS {};", schema_name));
}

void Config::ConfigureGlobal(duckdb::DatabaseInstance* db_instance) {
#ifdef __EMSCRIPTEN__
    // WASM: Client pre-attaches flock_storage before loading extension
    if (db_instance) {
        auto con = Config::GetConnection(db_instance);
        auto use_result = con.Query("USE flock_storage;");
        if (use_result->HasError()) {
            std::cerr << "Failed to USE flock_storage: " << use_result->GetError() << std::endl;
            return;
        }
        ConfigureTables(con, ConfigType::GLOBAL);
        con.Query("USE memory;");
    }
#else
    (void) db_instance;// unused on non-WASM
    auto con = Config::GetGlobalConnection();
    ConfigureTables(con, ConfigType::GLOBAL);
#endif
}

void Config::ConfigureLocal(duckdb::DatabaseInstance& db) {
    auto con = Config::GetConnection(&db);
    ConfigureTables(con, ConfigType::LOCAL);
#ifndef __EMSCRIPTEN__
    // Native only: Attach global storage database
    const std::string global_path = get_global_storage_path();
    auto result = con.Query(
            duckdb_fmt::format("ATTACH DATABASE '{}' AS flock_storage;", global_path));
    if (result->HasError()) {
        std::cerr << "Failed to attach flock_storage: " << result->GetError() << std::endl;
    }
#endif
    // WASM: Client pre-attaches flock_storage before loading extension
}

void Config::ConfigureTables(duckdb::Connection& con, const ConfigType type) {
    con.BeginTransaction();
    std::string schema = Config::get_schema_name();
    ConfigSchema(con, schema);
    ConfigModelTable(con, schema, type);
    ConfigPromptTable(con, schema, type);
    con.Commit();
}

void Config::Configure(duckdb::ExtensionLoader& loader) {
    Registry::Register(loader);
    SecretManager::Register(loader);
    auto& db = loader.GetDatabaseInstance();
    const auto db_path = db.config.options.database_path;
    const std::string& global_path = get_global_storage_path();

    // If the main database is already at the global storage path, no need to attach
    if (db_path == global_path) {
        // Main database IS the global storage - just configure tables
        auto con = GetConnection(&db);
        ConfigureTables(con, ConfigType::LOCAL);
        ConfigureTables(con, ConfigType::GLOBAL);
        return;
    }

    SetupGlobalStorageLocation(&db);
    ConfigureLocal(db);
#ifndef __EMSCRIPTEN__
    ConfigureGlobal(nullptr);
#else
    ConfigureGlobal(&db);
#endif
}

void Config::DetachFromGlobalStorage(duckdb::Connection& con) {
    con.Query("DETACH DATABASE flock_storage;");
}

bool Config::StorageAttachmentGuard::TryAttach(bool read_only) {
    try {
        Config::AttachToGlobalStorage(connection, read_only);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool Config::StorageAttachmentGuard::TryDetach() {
    try {
        Config::DetachFromGlobalStorage(connection);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void Config::StorageAttachmentGuard::Wait(int milliseconds) {
    auto start = std::chrono::steady_clock::now();
    auto duration = std::chrono::milliseconds(milliseconds);
    while (std::chrono::steady_clock::now() - start < duration) {
        // Busy-wait until the specified duration has elapsed
    }
}

Config::StorageAttachmentGuard::StorageAttachmentGuard(duckdb::Connection& con, bool read_only)
    : connection(con), attached(false) {
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        if (TryAttach(read_only)) {
            attached = true;
            return;
        }
        Wait(RETRY_DELAY_MS);
    }
    Config::AttachToGlobalStorage(connection, read_only);
    attached = true;
}


void Config::AttachToGlobalStorage(duckdb::Connection& con, bool read_only) {
    con.Query(duckdb_fmt::format("ATTACH DATABASE '{}' AS flock_storage {};",
                                 Config::get_global_storage_path(), read_only ? "(READ_ONLY)" : ""));
}


Config::StorageAttachmentGuard::~StorageAttachmentGuard() {
    if (attached) {
        try {
            Config::DetachFromGlobalStorage(connection);
        } catch (...) {
        }
    }
}

}// namespace flock
