#pragma once

#include <curl/curl.h>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>

struct Response {
    std::string text;
    bool is_error;
    std::string error_message;
};

// Simple curl Session inspired by CPR
class Session {
public:
    Session(const std::string& provider, bool throw_exception)
        : provider_(provider), throw_exception_{throw_exception} {
        initCurl();
        ignoreSSL();
    }

    Session(const std::string& provider, bool throw_exception, std::string proxy_url)
        : provider_(provider), throw_exception_{throw_exception} {
        initCurl();
        ignoreSSL();
        setProxyUrl(proxy_url);
    }

    ~Session() {
        curl_easy_cleanup(curl_);
        curl_global_cleanup();
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
        }
    }

    void initCurl() {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_ = curl_easy_init();
        if (curl_ == nullptr) {
            throw std::runtime_error("curl cannot initialize");// here we throw it shouldn't happen
        }
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    }

    void ignoreSSL() { curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L); }

    void setUrl(const std::string& url) { url_ = url; }

    void setToken(const std::string& token, const std::string& organization) {
        token_ = token;
        organization_ = organization;
    }
    void setProxyUrl(const std::string& url) {
        proxy_url_ = url;
        curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_url_.c_str());
    }

    void setBeta(const std::string& beta) { beta_ = beta; }

    void setBody(const std::string& data);
    void setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath,
                          const std::map<std::string, std::string>& fields);

    Response getPrepare();
    Response postPrepare(const std::string& contentType = "");
    Response postPrepareOllama(const std::string& contentType = "");
    Response deletePrepare();
    Response makeRequest(const std::string& contentType = "");
    void set_auth_header(struct curl_slist** headers_ptr);
    std::string easyEscape(const std::string& text);
    Response validOllamaModelsJson(const std::string& url);

private:
    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*) ptr, size * nmemb);
        return size * nmemb;
    }

private:
    CURL* curl_;
    CURLcode res_;
    curl_mime* mime_form_ = nullptr;
    std::string url_;
    std::string proxy_url_;
    std::string token_;
    std::string organization_;
    std::string beta_;
    std::string provider_;

    bool throw_exception_;
    std::mutex mutex_request_;
};

inline Response Session::validOllamaModelsJson(const std::string& url) {
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
}

inline void Session::setBody(const std::string& data) {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.length());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
    }
}

inline void Session::setMultiformPart(const std::pair<std::string, std::string>& fieldfield_and_filepath,
                                      const std::map<std::string, std::string>& fields) {
    // https://curl.se/libcurl/c/curl_mime_init.html
    if (curl_) {
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
            mime_form_ = nullptr;
        }
        curl_mimepart* field = nullptr;

        mime_form_ = curl_mime_init(curl_);

        field = curl_mime_addpart(mime_form_);
        curl_mime_name(field, fieldfield_and_filepath.first.c_str());
        curl_mime_filedata(field, fieldfield_and_filepath.second.c_str());

        for (const auto& field_pair: fields) {
            field = curl_mime_addpart(mime_form_);
            curl_mime_name(field, field_pair.first.c_str());
            curl_mime_data(field, field_pair.second.c_str(), CURL_ZERO_TERMINATED);
        }

        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, mime_form_);
    }
}

inline Response Session::getPrepare() {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
    }
    return makeRequest();
}

inline Response Session::postPrepare(const std::string& contentType) { return makeRequest(contentType); }

inline Response Session::postPrepareOllama(const std::string& contentType) {
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
}

inline Response Session::deletePrepare() {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    return makeRequest();
}

inline void Session::set_auth_header(struct curl_slist** headers_ptr) {
    auto headers = *headers_ptr;
    if (provider_ == "OpenAI") {
        std::string auth_str = "Authorization: Bearer " + token_;
        headers = curl_slist_append(headers, auth_str.c_str());
    } else if (provider_ == "Azure") {
        std::string auth_str = "api-key: " + token_;
        headers = curl_slist_append(headers, auth_str.c_str());
    }
}

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

    std::string organization_str = provider_ + "-Organization: ";
    if (!organization_.empty()) {
        organization_str += organization_;
        headers = curl_slist_append(headers, organization_str.c_str());
    }

    std::string beta_str = provider_ + "-Beta: ";
    if (!beta_.empty()) {
        beta_str += beta_;
        headers = curl_slist_append(headers, beta_.c_str());
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

inline std::string Session::easyEscape(const std::string& text) {
    char* encoded_output = curl_easy_escape(curl_, text.c_str(), static_cast<int>(text.length()));
    const auto str = std::string{encoded_output};
    curl_free(encoded_output);
    return str;
}
