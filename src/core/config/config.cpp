#include "flockmtl/core/config.hpp"
#include "filesystem.hpp"
#include "flockmtl/secret_manager/secret_manager.hpp"
#include <fmt/format.h>

namespace flockmtl {

duckdb::DatabaseInstance* Config::db;

std::string Config::get_schema_name() { return "flockmtl_config"; }

std::filesystem::path Config::get_global_storage_path() {
#ifdef _WIN32
    const char* homeDir = getenv("USERPROFILE");
#else
    const char* homeDir = getenv("HOME");
#endif
    if (homeDir == nullptr) {
        throw std::runtime_error("Could not find home directory");
    }
    return std::filesystem::path(homeDir) / ".duckdb" / "flockmtl_storage" / "flockmtl.db";
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
    const auto flockmtl_global_path = get_global_storage_path();
    const auto flockmtlDir = flockmtl_global_path.parent_path();
    if (!std::filesystem::exists(flockmtlDir)) {
        try {
            std::filesystem::create_directories(flockmtlDir);
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
    con.Query(
            duckdb_fmt::format("ATTACH DATABASE '{}' AS flockmtl_storage;", Config::get_global_storage_path().string()));
}

void Config::ConfigureTables(duckdb::Connection& con, const ConfigType type) {
    con.BeginTransaction();
    std::string schema = Config::get_schema_name();
    ConfigSchema(con, schema);
    ConfigModelTable(con, schema, type);
    ConfigPromptTable(con, schema, type);
    con.Commit();
}

void Config::Configure(duckdb::DatabaseInstance& db) {
    Registry::Register(db);
    SecretManager::Register(db);
    if (const auto db_path = db.config.options.database_path; db_path != get_global_storage_path().string()) {
        SetupGlobalStorageLocation();
        ConfigureGlobal();
        ConfigureLocal(db);
    }
}

}// namespace flockmtl
