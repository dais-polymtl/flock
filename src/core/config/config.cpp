#include "flock/core/config.hpp"
#include "filesystem.hpp"
#include "flock/secret_manager/secret_manager.hpp"
#include <chrono>
#include <fmt/format.h>

namespace flock {

duckdb::DatabaseInstance* Config::db;

std::string Config::get_schema_name() { return "flock_config"; }

std::filesystem::path Config::get_global_storage_path() {
#ifdef _WIN32
    const char* homeDir = getenv("USERPROFILE");
#else
    const char* homeDir = getenv("HOME");
#endif
    if (homeDir == nullptr) {
        throw std::runtime_error("Could not find home directory");
    }
    return std::filesystem::path(homeDir) / ".duckdb" / "flock_storage" / "flock.db";
}

duckdb::Connection Config::GetConnection(duckdb::DatabaseInstance* db) {
    if (db) {
        Config::db = db;
    }
    duckdb::Connection con(*Config::db);
    return con;
}

duckdb::Connection Config::GetGlobalConnection() {
    const duckdb::DuckDB db(Config::get_global_storage_path().string());
    duckdb::Connection con(*db.instance);
    return con;
}

void Config::SetupGlobalStorageLocation() {
    const auto flock_global_path = get_global_storage_path();
    const auto flockDir = flock_global_path.parent_path();
    if (!std::filesystem::exists(flockDir)) {
        try {
            std::filesystem::create_directories(flockDir);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directories: " << e.what() << std::endl;
        }
    }
}

void Config::ConfigSchema(duckdb::Connection& con, std::string& schema_name) {
    auto result = con.Query(duckdb_fmt::format(" SELECT * "
                                               "   FROM information_schema.schemata "
                                               "  WHERE schema_name = '{}'; ",
                                               schema_name));
    if (result->RowCount() == 0) {
        con.Query(duckdb_fmt::format("CREATE SCHEMA {};", schema_name));
    }
}

void Config::ConfigureGlobal() {
    auto con = Config::GetGlobalConnection();
    ConfigureTables(con, ConfigType::GLOBAL);
}

void Config::ConfigureLocal(duckdb::DatabaseInstance& db) {
    auto con = Config::GetConnection(&db);
    ConfigureTables(con, ConfigType::LOCAL);
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
    if (const auto db_path = db.config.options.database_path; db_path != get_global_storage_path().string()) {
        SetupGlobalStorageLocation();
        ConfigureGlobal();
        ConfigureLocal(db);
    }
}

void Config::AttachToGlobalStorage(duckdb::Connection& con, bool read_only) {
    con.Query(duckdb_fmt::format("ATTACH DATABASE '{}' AS flock_storage {};",
                                 Config::get_global_storage_path().string(), read_only ? "(READ_ONLY)" : ""));
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

Config::StorageAttachmentGuard::~StorageAttachmentGuard() {
    if (attached) {
        try {
            Config::DetachFromGlobalStorage(connection);
        } catch (...) {
        }
    }
}

}// namespace flock
