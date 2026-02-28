#pragma once

#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#ifdef __EMSCRIPTEN__
#include "wasm_http.hpp"
#include <nlohmann/json.hpp>
#else
#include <curl/curl.h>
#include <mutex>
#endif

struct Response {
    std::string text;
    bool is_error;
    std::string error_message;
};

// Simple curl Session inspired by CPR
class Session {
public:
    // Constructor/Destructor
    Session(const std::string& provider, bool throw_exception);
    Session(const std::string& provider, bool throw_exception, std::string proxy_url);
    ~Session();

    // Common interface
    void ignoreSSL();
    void setUrl(const std::string& url);
    void setToken(const std::string& token, const std::string& organization);
    void setProxyUrl(const std::string& url);
    void setBeta(const std::string& beta);
    void setBody(const std::string& data);
    void setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath,
                          const std::map<std::string, std::string>& fields);

    Response getPrepare();
    Response postPrepare(const std::string& contentType = "");
    Response deletePrepare();
    Response postPrepareOllama(const std::string& contentType = "");
    Response validOllamaModelsJson(const std::string& url);
    std::string easyEscape(const std::string& text);

private:
    std::string url_;
    std::string body_;
    std::string token_;
    std::string organization_;
    std::string beta_;
    std::string provider_;
    bool throw_exception_;

#ifdef __EMSCRIPTEN__
    Response makeWasmRequest(const char* method, const std::string& contentType);
#else
    // Native-only members
    CURL* curl_;
    CURLcode res_;
    curl_mime* mime_form_ = nullptr;
    std::string proxy_url_;
    std::mutex mutex_request_;

    void initCurl();
    // Native-specific helpers
    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*) ptr, size * nmemb);
        return size * nmemb;
    }
    Response makeRequest(const std::string& contentType = "");
    void set_auth_header(struct curl_slist** headers_ptr);
#endif
};


inline void Session::setUrl(const std::string& url) { url_ = url; }
inline void Session::setBeta(const std::string& beta) { beta_ = beta; }
inline void Session::setToken(const std::string& token, const std::string& organization) {
    token_ = token;
    organization_ = organization;
}

inline Session::Session(const std::string& provider, bool throw_exception)
    : provider_(provider), throw_exception_(throw_exception) {
#ifndef __EMSCRIPTEN__
    initCurl();
    ignoreSSL();
#endif
}

inline Session::Session(const std::string& provider, bool throw_exception, std::string proxy_url)
    : provider_(provider), throw_exception_(throw_exception) {
    // Proxy is not supported in WASM
#ifndef __EMSCRIPTEN__
    initCurl();
#else
    ignoreSSL();
    setProxyUrl(proxy_url);
#endif
}

inline Session::~Session() {
#ifndef __EMSCRIPTEN__
    curl_easy_cleanup(curl_);
    curl_global_cleanup();
    if (mime_form_ != nullptr) {
        curl_mime_free(mime_form_);
    }
#endif
}


inline std::string Session::easyEscape(const std::string& text) {
#ifndef __EMSCRIPTEN__

    char* encoded_output = curl_easy_escape(curl_, text.c_str(), static_cast<int>(text.length()));
    std::string str = std::string{encoded_output};
    curl_free(encoded_output);
    return str;
#else
    std::string result;
    for (char c: text) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char) c);
            result += buf;
        }
    }
    return result;
#endif
}

#ifdef __EMSCRIPTEN__
inline Response Session::makeWasmRequest(const char* method, const std::string& contentType) {
    // Build headers as JSON object
    std::string headers_json = "{";
    bool first = true;

    const auto addHeader = [&](const std::string& key, const std::string& value) {
        if (!first) headers_json += ",";
        headers_json += "\"" + key + "\":\"" + value + "\"";
        first = false;
    };

    if (!contentType.empty()) {
        addHeader("Content-Type", contentType);
    }

    if (!token_.empty()) {
        if (provider_ == "OpenAI") {
            addHeader("Authorization", "Bearer " + token_);
        } else if (provider_ == "Azure") {
            addHeader("api-key", token_);
        } else if (provider_ == "Anthropic") {
            addHeader("x-api-key", token_);
            addHeader("anthropic-version", "2023-06-01");
        }
    }

    if (!organization_.empty()) {
        addHeader(provider_ + "-Organization", organization_);
    }

    if (!beta_.empty()) {
        addHeader(provider_ + "-Beta", beta_);
    }

    headers_json += "}";

    // Make the request via JavaScript
    char* result = wasm_http_request(method, url_.c_str(), body_.c_str(), headers_json.c_str());
    std::string result_str(result);
    free(result);

    Response response;
    try {
        auto json_result = nlohmann::json::parse(result_str);

        int status = json_result.value("status", 0);
        std::string response_text = json_result.value("response", "");
        bool has_error = json_result.contains("error");

        if (has_error || status == 0) {
            response.is_error = true;
            response.error_message = provider_ + " HTTP request failed: " + result_str;
            if (throw_exception_) {
                throw std::runtime_error(response.error_message);
            }
        } else if (status >= 200 && status < 300) {
            response.text = response_text;
            response.is_error = false;
        } else {
            response.is_error = true;
            response.error_message = provider_ + " HTTP " + std::to_string(status) + ": " + response_text;
            if (throw_exception_) {
                throw std::runtime_error(response.error_message);
            }
        }
    } catch (const std::exception& e) {
        response.is_error = true;
        response.error_message = provider_ + " Error parsing response: " + e.what();
        if (throw_exception_) {
            throw;
        }
    }

    return response;
}
#endif


#ifndef __EMSCRIPTEN__
inline void Session::initCurl() {
    curl_ = curl_easy_init();
    if (curl_ == nullptr) {
        throw std::runtime_error("curl cannot initialize");
    }
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1);
}
#endif

inline void Session::ignoreSSL() {
#ifndef __EMSCRIPTEN__
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
}

inline void Session::setProxyUrl(const std::string& url) {
#ifndef __EMSCRIPTEN__
    proxy_url_ = url;
    curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_url_.c_str());
#endif
}

inline void Session::setBody(const std::string& data) {
#ifndef __EMSCRIPTEN__
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.length());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
    }
#else
    body_ = data;
#endif
}

inline void Session::setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath,
                                      const std::map<std::string, std::string>& fields) {
#ifndef __EMSCRIPTEN__
    if (curl_) {
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
            mime_form_ = nullptr;
        }
        curl_mimepart* field = nullptr;

        mime_form_ = curl_mime_init(curl_);

        field = curl_mime_addpart(mime_form_);
        curl_mime_name(field, filefield_and_filepath.first.c_str());
        curl_mime_filedata(field, filefield_and_filepath.second.c_str());

        for (const auto& field_pair: fields) {
            field = curl_mime_addpart(mime_form_);
            curl_mime_name(field, field_pair.first.c_str());
            curl_mime_data(field, field_pair.second.c_str(), CURL_ZERO_TERMINATED);
        }

        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, mime_form_);
    }
#else
    throw std::runtime_error("Multipart form data not supported in WASM");
#endif
}

inline Response Session::getPrepare() {
#ifndef __EMSCRIPTEN__
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
    }
    return makeRequest();
#else

    return makeWasmRequest("GET", "");

#endif
}

inline Response Session::postPrepare(const std::string& contentType) {
#ifndef __EMSCRIPTEN__
    return makeRequest(contentType);
#else
    return makeWasmRequest("POST", contentType.empty() ? "application/json" : contentType);
#endif
}

inline Response Session::deletePrepare() {
#ifndef __EMSCRIPTEN__
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    return makeRequest();
#else
    return makeWasmRequest("DELETE", "");
#endif
}

inline Response Session::postPrepareOllama(const std::string& contentType) {
#ifndef __EMSCRIPTEN__
    std::lock_guard<std::mutex> lock(mutex_request_);

    struct curl_slist* headers = NULL;
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

    std::string response_string;
    std::string header_string;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_string);

    res_ = curl_easy_perform(curl_);

    bool is_error = false;
    std::string error_msg{};
    if (res_ != CURLE_OK) {
        is_error = true;
        error_msg = provider_ + " curl_easy_perform() failed: " + std::string{curl_easy_strerror(res_)};
        if (throw_exception_) {
            throw std::runtime_error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
    }
    return {response_string, is_error, error_msg};
#else
    return makeWasmRequest("POST", contentType.empty() ? "application/json" : contentType);
#endif
}

inline Response Session::validOllamaModelsJson(const std::string& url) {
#ifndef __EMSCRIPTEN__
    std::lock_guard<std::mutex> lock(mutex_request_);

    struct curl_slist* headers = NULL;
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());

    std::string response_string;
    std::string header_string;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_string);

    res_ = curl_easy_perform(curl_);
    bool is_error = false;
    std::string error_msg{};
    if (res_ != CURLE_OK) {
        is_error = true;
        error_msg = " curl_easy_perform() failed: " + std::string{curl_easy_strerror(res_)};
        if (throw_exception_) {
            throw std::runtime_error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
    }
    return {response_string, is_error, error_msg};
#else
    url_ = url;
    return makeWasmRequest("GET", "");
#endif
}

#ifndef __EMSCRIPTEN__
inline Response Session::makeRequest(const std::string& contentType) {
    std::lock_guard<std::mutex> lock(mutex_request_);

    struct curl_slist* headers = NULL;

    std::string content_type_str = "Content-Type: ";
    if (!contentType.empty()) {
        content_type_str += contentType;
        headers = curl_slist_append(headers, content_type_str.c_str());
        if (contentType == "multipart/form-data") {
            headers = curl_slist_append(headers, "Expect:");
        }
    }

    set_auth_header(&headers);

    if (provider_ == "Anthropic") {
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    }

    std::string organization_str = provider_ + "-Organization: ";
    if (!organization_.empty()) {
        organization_str += organization_;
        headers = curl_slist_append(headers, organization_str.c_str());
    }

    std::string beta_str = provider_ + "-Beta: ";
    if (!beta_.empty()) {
        beta_str += beta_;
        headers = curl_slist_append(headers, beta_str.c_str());
    }

    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

    std::string response_string;
    std::string header_string;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_string);

    res_ = curl_easy_perform(curl_);

    bool is_error = false;
    std::string error_msg{};
    if (res_ != CURLE_OK) {
        is_error = true;
        error_msg = provider_ + " curl_easy_perform() failed: " + std::string{curl_easy_strerror(res_)};
        if (throw_exception_) {
            throw std::runtime_error(error_msg);
        } else {
            std::cerr << error_msg << '\n';
        }
    }

    return {response_string, is_error, error_msg};
}

inline void Session::set_auth_header(struct curl_slist** headers_ptr) {
    if (provider_ == "OpenAI") {
        std::string auth_str = "Authorization: Bearer " + token_;
        *headers_ptr = curl_slist_append(*headers_ptr, auth_str.c_str());
    } else if (provider_ == "Azure") {
        std::string auth_str = "api-key: " + token_;
        *headers_ptr = curl_slist_append(*headers_ptr, auth_str.c_str());
    } else if (provider_ == "Anthropic") {
        std::string auth_str = "x-api-key: " + token_;
        *headers_ptr = curl_slist_append(*headers_ptr, auth_str.c_str());
    }
}
#endif
