#include "filesystem.hpp"
#include "flock/core/config.hpp"

namespace flock {

std::string Config::get_default_models_table_name() { return "FLOCKMTL_MODEL_DEFAULT_INTERNAL_TABLE"; }

std::string Config::get_user_defined_models_table_name() { return "FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"; }

void Config::SetupDefaultModelsConfig(duckdb::Connection& con, std::string& schema_name) {
    const std::string table_name = Config::get_default_models_table_name();
    // Ensure schema exists
    auto result = con.Query(duckdb_fmt::format(" SELECT table_name "
                                               "   FROM information_schema.tables "
                                               "  WHERE table_schema = '{}' "
                                               "    AND table_name = '{}'; ",
                                               schema_name, table_name));
    if (result->RowCount() == 0) {
        con.Query(duckdb_fmt::format(" INSTALL JSON; "
                                     " LOAD JSON; "
                                     " CREATE TABLE {}.{} ( "
                                     " model_name VARCHAR NOT NULL PRIMARY KEY, "
                                     " model VARCHAR NOT NULL, "
                                     " provider_name VARCHAR NOT NULL, "
                                     " model_args JSON DEFAULT '{{}}'"
                                     " ); ",
                                     schema_name, table_name));

        con.Query(duckdb_fmt::format(
                "INSERT INTO {}.{} (model_name, model, provider_name) "
                "VALUES "
                "('default', 'gpt-4o-mini', 'openai'), "
                "('gpt-4o-mini', 'gpt-4o-mini', 'openai'), "
                "('gpt-4o', 'gpt-4o', 'openai'), "
                "('gpt-4o-transcribe', 'gpt-4o-transcribe', 'openai'),"
                "('gpt-4o-mini-transcribe', 'gpt-4o-mini-transcribe', 'openai'),"
                "('text-embedding-3-large', 'text-embedding-3-large', 'openai'), "
                "('text-embedding-3-small', 'text-embedding-3-small', 'openai');",
                schema_name, table_name));
    }
}

void Config::SetupUserDefinedModelsConfig(duckdb::Connection& con, std::string& schema_name) {
    const std::string table_name = Config::get_user_defined_models_table_name();
    // Ensure schema exists
    auto result = con.Query(duckdb_fmt::format(" SELECT table_name "
                                               "   FROM information_schema.tables "
                                               "  WHERE table_schema = '{}' "
                                               "    AND table_name = '{}'; ",
                                               schema_name, table_name));
    if (result->RowCount() == 0) {
        con.Query(duckdb_fmt::format(" INSTALL JSON; "
                                     "   LOAD JSON; "
                                     " CREATE TABLE {}.{} ( "
                                     " model_name VARCHAR NOT NULL PRIMARY KEY, "
                                     " model VARCHAR NOT NULL, "
                                     " provider_name VARCHAR NOT NULL, "
                                     " model_args JSON NOT NULL"
                                     " ); ",
                                     schema_name, table_name));
    }
}

void Config::ConfigModelTable(duckdb::Connection& con, std::string& schema_name, const ConfigType type) {
    if (type == ConfigType::GLOBAL) {
        SetupDefaultModelsConfig(con, schema_name);
    }
    SetupUserDefinedModelsConfig(con, schema_name);
}

}// namespace flock
