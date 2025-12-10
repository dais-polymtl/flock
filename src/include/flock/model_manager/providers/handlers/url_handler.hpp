#pragma once

#include "flock/core/common.hpp"
#include <cstdio>
#include <curl/curl.h>
#include <random>
#include <sstream>
#include <string>

namespace flock {

class URLHandler {
public:
    // Extract file extension from URL
    static std::string ExtractFileExtension(const std::string& url) {
        size_t last_dot = url.find_last_of('.');
        size_t last_slash = url.find_last_of('/');
        if (last_dot != std::string::npos && (last_slash == std::string::npos || last_dot > last_slash)) {
            size_t query_pos = url.find_first_of('?', last_dot);
            if (query_pos != std::string::npos) {
                return url.substr(last_dot, query_pos - last_dot);
            } else {
                return url.substr(last_dot);
            }
        }
        return "";// No extension found
    }

    // Generate a unique temporary filename with extension
    static std::string GenerateTempFilename(const std::string& extension) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        std::ostringstream oss;
        oss << "/tmp/flock_";
        for (int i = 0; i < 16; ++i) {
            oss << std::hex << dis(gen);
        }
        oss << extension;
        return oss.str();
    }

    // Check if the given path is a URL
    static bool IsUrl(const std::string& path) {
        return path.find("http://") == 0 || path.find("https://") == 0;
    }

    // Validate file exists and is not empty
    static bool ValidateFile(const std::string& file_path) {
        FILE* f = fopen(file_path.c_str(), "rb");
        if (!f) {
            return false;
        }
        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        fclose(f);
        return file_size > 0;
    }

    // Download file from URL to temporary location
    static std::string DownloadFileToTemp(const std::string& url) {
        std::string extension = ExtractFileExtension(url);
        // If no extension found, try to infer from content-type or use empty extension
        std::string temp_filename = GenerateTempFilename(extension);

        // Download file using curl
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "";
        }

        FILE* file = fopen(temp_filename.c_str(), "wb");
        if (!file) {
            curl_easy_cleanup(curl);
            return "";
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(
                curl, CURLOPT_WRITEFUNCTION, +[](void* ptr, size_t size, size_t nmemb, void* stream) -> size_t { return fwrite(ptr, size, nmemb, static_cast<FILE*>(stream)); });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        fclose(file);
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || response_code != 200) {
            std::remove(temp_filename.c_str());
            return "";
        }

        return temp_filename;
    }

    // Helper struct to return file path and temp file flag
    struct FilePathResult {
        std::string file_path;
        bool is_temp_file;
    };

    // Resolve file path: download if URL, validate, and return result
    // Throws std::runtime_error if download or validation fails
    static FilePathResult ResolveFilePath(const std::string& file_path_or_url) {
        FilePathResult result;

        if (IsUrl(file_path_or_url)) {
            result.file_path = DownloadFileToTemp(file_path_or_url);
            if (result.file_path.empty()) {
                throw std::runtime_error("Failed to download file: " + file_path_or_url);
            }
            result.is_temp_file = true;
        } else {
            result.file_path = file_path_or_url;
            result.is_temp_file = false;
        }

        if (!ValidateFile(result.file_path)) {
            if (result.is_temp_file) {
                std::remove(result.file_path.c_str());
            }
            throw std::runtime_error("Invalid file: " + file_path_or_url);
        }

        return result;
    }
};

}// namespace flock
