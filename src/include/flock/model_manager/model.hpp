#pragma once

#include "fmt/format.h"
#include <functional>
#include <utility>

#include "duckdb/main/connection.hpp"
#include "flock/core/common.hpp"
#include "flock/core/config.hpp"
#include "flock/model_manager/providers/adapters/anthropic.hpp"
#include "flock/model_manager/providers/adapters/azure.hpp"
#include "flock/model_manager/providers/adapters/ollama.hpp"
#include "flock/model_manager/providers/adapters/openai.hpp"
#include "flock/model_manager/providers/handlers/ollama.hpp"
#include "flock/model_manager/repository.hpp"
#include <nlohmann/json.hpp>

namespace flock {

enum class ExecutionMode {
    SYNC,
    ASYNC
};

class Model {
public:
    explicit Model(const nlohmann::json& model_json);
    explicit Model() = default;
    void AddCompletionRequest(const std::string& prompt, const int num_output_tuples, OutputType output_type = OutputType::STRING, const nlohmann::json& media_data = nlohmann::json::object());
    void AddEmbeddingRequest(const std::vector<std::string>& inputs);
    void AddTranscriptionRequest(const nlohmann::json& audio_files);
    std::vector<nlohmann::json> CollectCompletions(const std::string& contentType = "application/json");
    std::vector<nlohmann::json> CollectEmbeddings(const std::string& contentType = "application/json");
    std::vector<nlohmann::json> CollectTranscriptions(const std::string& contentType = "multipart/form-data");
    ModelDetails GetModelDetails();
    nlohmann::json GetModelDetailsAsJson() const;

    // Static helper method for binders to resolve model details to JSON
    static nlohmann::json ResolveModelDetailsToJson(const nlohmann::json& user_model_json);

    // Factory function type for creating mock providers
    using MockProviderFactory = std::function<std::shared_ptr<IProvider>()>;

    // Set a factory to create fresh mock providers (each Model gets its own instance)
    static void SetMockProviderFactory(MockProviderFactory factory) {
        mock_provider_factory_ = std::move(factory);
    }

    // Legacy: Set a shared mock provider (for backward compatibility - less safe for parallel tests)
    static void SetMockProvider(const std::shared_ptr<IProvider>& mock_provider) {
        mock_provider_ = mock_provider;
    }

    static void ResetMockProvider() {
        mock_provider_ = nullptr;
        mock_provider_factory_ = nullptr;
    }

    std::shared_ptr<IProvider>
            provider_;

private:
    ModelDetails model_details_;
    inline static std::shared_ptr<IProvider> mock_provider_ = nullptr;
    inline static MockProviderFactory mock_provider_factory_ = nullptr;
    void ConstructProvider();
    void LoadModelDetails(const nlohmann::json& model_json);
    static std::tuple<std::string, std::string, nlohmann::basic_json<>> GetQueriedModel(const std::string& model_name);
    std::string GetSecret(const std::string& secret_name);
};

}// namespace flock
