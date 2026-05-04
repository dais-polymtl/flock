#pragma once

#include "flock/core/config.hpp"
#include "nlohmann/json.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace flock {

inline std::string ReadCommandOutput(const std::string& command) {
    std::array<char, 256> buffer {};
    std::string output;

    auto pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to query Ollama models");
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    const auto status = pclose(pipe);
    if (status != 0) {
        throw std::runtime_error("Failed to query Ollama models. Is Ollama running?");
    }

    return output;
}

inline std::string GetOllamaTestModelName() {
    const auto* model_name = std::getenv("FLOCK_OLLAMA_TEST_MODEL");
    if (model_name && model_name[0] != '\0') {
        return model_name;
    }

    const auto response = ReadCommandOutput("curl -fsS http://127.0.0.1:11434/api/tags");
    const auto tags = nlohmann::json::parse(response);
    if (!tags.contains("models") || !tags["models"].is_array() || tags["models"].empty()) {
        throw std::runtime_error("No Ollama models are downloaded. Pull a model or set FLOCK_OLLAMA_TEST_MODEL.");
    }

    return tags["models"][0]["name"].get<std::string>();
}

inline void SeedOllamaTestModel(duckdb::Connection& con) {
    const auto model_name = GetOllamaTestModelName();
    con.Query(" DELETE FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
              "  WHERE provider_name = 'ollama';");
    con.Query(" INSERT INTO flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
              "        (model_name, model, provider_name, model_args) "
              " VALUES ('" +
              model_name + "', '" + model_name + "', 'ollama', '{}');");
}

}// namespace flock
