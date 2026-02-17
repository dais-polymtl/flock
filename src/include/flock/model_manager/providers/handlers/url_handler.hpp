#pragma once

#include "flock/core/common.hpp"
#include "flock/core/config.hpp"
#include <cstdio>
#include <curl/curl.h>
#include <filesystem>
#include <random>
#include <regex>
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
        // Get the flock storage directory (parent of the database file)
        std::filesystem::path storage_dir = Config::get_global_storage_path().parent_path();

        // Ensure the directory exists
        if (!std::filesystem::exists(storage_dir)) {
            std::filesystem::create_directories(storage_dir);
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        std::ostringstream filename;
        filename << "flock_";
        for (int i = 0; i < 16; ++i) {
            filename << std::hex << dis(gen);
        }
        filename << extension;

        // Use filesystem path for proper cross-platform path handling
        std::filesystem::path temp_path = storage_dir / filename.str();
        return temp_path.string();
    }

    // Check if the given path is a URL using regex
    static bool IsUrl(const std::string& path) {
        // Regex pattern to match URLs: http:// or https://
        static const std::regex url_pattern(R"(^https?://)");
        return std::regex_search(path, url_pattern);
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
    // Supports http:// and https:// URLs
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

    // Read file contents and convert to base64
    // Returns empty string if file cannot be read
    static std::string ReadFileToBase64(const std::string& file_path) {
        FILE* file = fopen(file_path.c_str(), "rb");
        if (!file) {
            return "";
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size <= 0) {
            fclose(file);
            return "";
        }

        // Read file content
        std::vector<unsigned char> buffer(file_size);
        size_t bytes_read = fread(buffer.data(), 1, file_size, file);
        fclose(file);

        if (bytes_read != static_cast<size_t>(file_size)) {
            return "";
        }

        // Base64 encoding table
        static const char base64_chars[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        result.reserve(((file_size + 2) / 3) * 4);

        for (size_t i = 0; i < bytes_read; i += 3) {
            unsigned int octet_a = buffer[i];
            unsigned int octet_b = (i + 1 < bytes_read) ? buffer[i + 1] : 0;
            unsigned int octet_c = (i + 2 < bytes_read) ? buffer[i + 2] : 0;

            unsigned int triple = (octet_a << 16) + (octet_b << 8) + octet_c;

            result.push_back(base64_chars[(triple >> 18) & 0x3F]);
            result.push_back(base64_chars[(triple >> 12) & 0x3F]);
            result.push_back((i + 1 < bytes_read) ? base64_chars[(triple >> 6) & 0x3F] : '=');
            result.push_back((i + 2 < bytes_read) ? base64_chars[triple & 0x3F] : '=');
        }

        return result;
    }

    // Helper struct to return base64 content and temp file flag
    struct Base64Result {
        std::string base64_content;
        bool is_temp_file;
        std::string temp_file_path;
    };

    // Resolve file path or URL, read contents and convert to base64
    // If input is URL, downloads to temp file first
    // Returns base64 content and temp file info for cleanup
    // Throws std::runtime_error if file cannot be processed
    static Base64Result ResolveFileToBase64(const std::string& file_path_or_url) {
        Base64Result result;
        result.is_temp_file = false;

        std::string file_path;
        if (IsUrl(file_path_or_url)) {
            file_path = DownloadFileToTemp(file_path_or_url);
            if (file_path.empty()) {
                throw std::runtime_error("Failed to download file: " + file_path_or_url);
            }
            result.is_temp_file = true;
            result.temp_file_path = file_path;
        } else {
            file_path = file_path_or_url;
        }

        if (!ValidateFile(file_path)) {
            if (result.is_temp_file) {
                std::remove(file_path.c_str());
            }
            throw std::runtime_error("Invalid file: " + file_path_or_url);
        }

        result.base64_content = ReadFileToBase64(file_path);
        if (result.base64_content.empty()) {
            if (result.is_temp_file) {
                std::remove(file_path.c_str());
            }
            throw std::runtime_error("Failed to read file: " + file_path_or_url);
        }

        // Cleanup temp file after reading
        if (result.is_temp_file) {
            std::remove(file_path.c_str());
            result.temp_file_path.clear();
        }

        return result;
    }
};

}// namespace flock
