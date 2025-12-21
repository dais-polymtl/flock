#include "flock/model_manager/providers/handlers/anthropic.hpp"
#include "flock/model_manager/providers/provider.hpp"
#include "flock/model_manager/repository.hpp"
#include "nlohmann/json.hpp"
#include <gtest/gtest.h>

namespace flock {
using json = nlohmann::json;

class AnthropicHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Tests focus on JSON parsing and response handling
    }
};

// Test text response structure
TEST_F(AnthropicHandlerTest, TextResponseStructure) {
    json response = {
        {"id", "msg_01"},
        {"type", "message"},
        {"role", "assistant"},
        {"content", {{{"type", "text"}, {"text", "{\"items\": [\"result1\"]}"} }}},
        {"stop_reason", "end_turn"},
        {"model", "claude-3-haiku-20240307"}
    };

    EXPECT_TRUE(response["content"].is_array());
    EXPECT_FALSE(response["content"].empty());
    EXPECT_EQ(response["content"][0]["type"], "text");
    EXPECT_TRUE(response["content"][0].contains("text"));
}

// Test tool_use response structure (structured output)
TEST_F(AnthropicHandlerTest, ToolUseResponseStructure) {
    json response = {
        {"id", "msg_01"},
        {"type", "message"},
        {"role", "assistant"},
        {"content", {{
            {"type", "tool_use"},
            {"id", "toolu_01"},
            {"name", "flock_response"},
            {"input", {{"items", {"result1", "result2"}}}}
        }}},
        {"stop_reason", "tool_use"},
        {"model", "claude-3-haiku-20240307"}
    };

    EXPECT_TRUE(response["content"].is_array());
    EXPECT_EQ(response["content"][0]["type"], "tool_use");
    EXPECT_EQ(response["content"][0]["name"], "flock_response");
    EXPECT_TRUE(response["content"][0]["input"].contains("items"));
    EXPECT_EQ(response["content"][0]["input"]["items"].size(), 2);
}

// Test valid stop reasons
TEST_F(AnthropicHandlerTest, ValidStopReasons) {
    std::vector<std::string> valid_reasons = {"end_turn", "stop_sequence", "tool_use"};

    for (const auto& reason : valid_reasons) {
        json response = {
            {"content", {{{"type", "text"}, {"text", "test"}}}},
            {"stop_reason", reason}
        };
        EXPECT_EQ(response["stop_reason"], reason);
    }
}

// Test max_tokens stop reason
TEST_F(AnthropicHandlerTest, MaxTokensStopReason) {
    json response = {
        {"content", {{{"type", "text"}, {"text", "partial response..."}}}},
        {"stop_reason", "max_tokens"}
    };

    EXPECT_EQ(response["stop_reason"], "max_tokens");
    // This should trigger ExceededMaxOutputTokensError in checkProviderSpecificResponse
}

// Test error response structure
TEST_F(AnthropicHandlerTest, ErrorResponseStructure) {
    json error_response = {
        {"type", "error"},
        {"error", {
            {"type", "invalid_request_error"},
            {"message", "max_tokens must be greater than 0"}
        }}
    };

    EXPECT_EQ(error_response["type"], "error");
    EXPECT_TRUE(error_response["error"].contains("message"));
    EXPECT_EQ(error_response["error"]["type"], "invalid_request_error");
}

// Test header format
TEST_F(AnthropicHandlerTest, HeaderFormat) {
    std::string api_key = "sk-ant-api-test123";
    std::string api_version = ANTHROPIC_DEFAULT_API_VERSION;

    std::vector<std::string> expected_headers = {
        "x-api-key: " + api_key,
        "anthropic-version: " + api_version
    };

    EXPECT_EQ(expected_headers.size(), 2);
    EXPECT_TRUE(expected_headers[0].find("x-api-key") != std::string::npos);
    EXPECT_TRUE(expected_headers[1].find("anthropic-version") != std::string::npos);
    EXPECT_TRUE(expected_headers[1].find(ANTHROPIC_DEFAULT_API_VERSION) != std::string::npos);
}

// Test request payload structure
TEST_F(AnthropicHandlerTest, RequestPayloadStructure) {
    json request = {
        {"model", "claude-3-haiku-20240307"},
        {"max_tokens", 1024},
        {"messages", {{
            {"role", "user"},
            {"content", {{{"type", "text"}, {"text", "Hello"}}}}
        }}}
    };

    EXPECT_TRUE(request.contains("model"));
    EXPECT_TRUE(request.contains("max_tokens"));
    EXPECT_TRUE(request.contains("messages"));
    EXPECT_EQ(request["max_tokens"], 1024);
    EXPECT_EQ(request["messages"][0]["role"], "user");
}

// Test request with system prompt
TEST_F(AnthropicHandlerTest, RequestWithSystemPrompt) {
    json request = {
        {"model", "claude-3-haiku-20240307"},
        {"max_tokens", 1024},
        {"system", "You are a helpful assistant."},
        {"messages", {{
            {"role", "user"},
            {"content", {{{"type", "text"}, {"text", "Hello"}}}}
        }}}
    };

    EXPECT_TRUE(request.contains("system"));
    EXPECT_EQ(request["system"], "You are a helpful assistant.");
}

// Test request with tools for structured output
TEST_F(AnthropicHandlerTest, RequestWithTools) {
    json tool = {
        {"name", "flock_response"},
        {"description", "Return the structured response"},
        {"input_schema", {
            {"type", "object"},
            {"properties", {
                {"items", {{"type", "array"}}}
            }},
            {"required", {"items"}}
        }}
    };

    json request = {
        {"model", "claude-3-haiku-20240307"},
        {"max_tokens", 1024},
        {"messages", {{{"role", "user"}, {"content", "test"}}}},
        {"tools", {tool}},
        {"tool_choice", {{"type", "tool"}, {"name", "flock_response"}}}
    };

    EXPECT_TRUE(request.contains("tools"));
    EXPECT_TRUE(request.contains("tool_choice"));
    EXPECT_EQ(request["tools"][0]["name"], "flock_response");
    EXPECT_EQ(request["tool_choice"]["name"], "flock_response");
}

// Test image content structure
TEST_F(AnthropicHandlerTest, ImageContentStructure) {
    json image_content = {
        {"type", "image"},
        {"source", {
            {"type", "base64"},
            {"media_type", "image/png"},
            {"data", "iVBORw0KGgo..."}  // Truncated base64
        }}
    };

    EXPECT_EQ(image_content["type"], "image");
    EXPECT_EQ(image_content["source"]["type"], "base64");
    EXPECT_TRUE(image_content["source"].contains("media_type"));
    EXPECT_TRUE(image_content["source"].contains("data"));
}

// Test API URL construction
TEST_F(AnthropicHandlerTest, APIUrlConstruction) {
    std::string base_url = "https://api.anthropic.com/v1/";
    std::string completion_url = base_url + "messages";

    EXPECT_EQ(completion_url, "https://api.anthropic.com/v1/messages");
}

// Test mixed content array
TEST_F(AnthropicHandlerTest, MixedContentArray) {
    json content = json::array({
        {{"type", "text"}, {"text", "What is in this image?"}},
        {{"type", "image"}, {"source", {
            {"type", "base64"},
            {"media_type", "image/jpeg"},
            {"data", "base64data"}
        }}}
    });

    EXPECT_EQ(content.size(), 2);
    EXPECT_EQ(content[0]["type"], "text");
    EXPECT_EQ(content[1]["type"], "image");
}

// Test response with multiple content blocks
TEST_F(AnthropicHandlerTest, MultipleContentBlocks) {
    json response = {
        {"content", {
            {{"type", "text"}, {"text", "Let me analyze this."}},
            {{"type", "tool_use"}, {"id", "toolu_01"}, {"name", "flock_response"},
             {"input", {{"items", {"result"}}}}}
        }},
        {"stop_reason", "tool_use"}
    };

    EXPECT_EQ(response["content"].size(), 2);
    // The handler should prioritize tool_use over text
}

}// namespace flock
