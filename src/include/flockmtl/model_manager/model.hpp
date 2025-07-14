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

enum class ExecutionMode {
    SYNC,
    ASYNC
};

class Model {
public:
    explicit Model(const nlohmann::json& model_json);
    explicit Model() = default;
    void AddCompletionRequest(const std::string& prompt, bool json_response = true,
                              OutputType output_type = OutputType::STRING);
    void AddEmbeddingRequest(const std::vector<std::string>& inputs);
    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json");
    std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json");
    ModelDetails GetModelDetails();

    static void SetMockProvider(const std::shared_ptr<IProvider>& mock_provider) {
        mock_provider_ = mock_provider;
    }
    static void ResetMockProvider() {
        mock_provider_ = nullptr;
    }

    std::shared_ptr<IProvider>
            provider_;

private:
    ModelDetails model_details_;
    inline static std::shared_ptr<IProvider> mock_provider_ = nullptr;
    void ConstructProvider();
    void LoadModelDetails(const nlohmann::json& model_json);
    static std::tuple<std::string, std::string, nlohmann::basic_json<>> GetQueriedModel(const std::string& model_name);
    std::string GetSecret(const std::string& secret_name);
};

}// namespace flockmtl
