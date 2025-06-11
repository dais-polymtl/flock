#pragma once

#include "fmt/format.h"
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "duckdb/main/connection.hpp"
#include "flockmtl/core/config.hpp"
#include "flockmtl/model_manager/providers/adapters/azure.hpp"
#include "flockmtl/model_manager/providers/adapters/ollama.hpp"
#include "flockmtl/model_manager/providers/adapters/openai.hpp"
#include "flockmtl/model_manager/providers/handlers/ollama.hpp"
#include "flockmtl/model_manager/repository.hpp"

namespace flockmtl {

class Model {
public:
    explicit Model(const nlohmann::json& model_json);
    explicit Model() = default;
    nlohmann::json CallComplete(const std::string& prompt, const bool json_response = true);
    nlohmann::json CallEmbedding(const std::vector<std::string>& inputs);
    ModelDetails GetModelDetails();

    static void SetMockProvider(const std::shared_ptr<IProvider>& mock_provider) {
        mock_provider_ = mock_provider;
    }
    static void ResetMockProvider() {
        mock_provider_ = nullptr;
    }

private:
    std::shared_ptr<IProvider>
            provider_;
    ModelDetails model_details_;
    inline static std::shared_ptr<IProvider> mock_provider_ = nullptr;
    void ConstructProvider();
    void LoadModelDetails(const nlohmann::json& model_json);
    std::tuple<std::string, std::string, int32_t, int32_t> GetQueriedModel(const std::string& model_name);
    std::string GetSecret(const std::string& secret_name);
};

}// namespace flockmtl
