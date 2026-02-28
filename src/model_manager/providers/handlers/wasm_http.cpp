#ifdef __EMSCRIPTEN__

#include <cstdlib>
#include <emscripten.h>
#include <string>

// JavaScript XMLHttpRequest wrapper - works synchronously in web workers
EM_JS(char*, wasm_http_request_impl, (const char* method, const char* url, const char* body, const char* headers_json), {
    try {
        var xhr = new XMLHttpRequest();
        xhr.open(UTF8ToString(method), UTF8ToString(url), false);  // false = synchronous

        // Parse and set headers
        var headersStr = UTF8ToString(headers_json);
        if (headersStr && headersStr !== "{}") {
            try {
                var headers = JSON.parse(headersStr);
                for (var key in headers) {
                    if (headers.hasOwnProperty(key)) {
                        xhr.setRequestHeader(key, headers[key]);
                    }
                }
            } catch (e) {
                // Ignore header parsing errors
            }
        }

        var bodyStr = UTF8ToString(body);
        if (bodyStr && bodyStr.length > 0) {
            xhr.send(bodyStr);
        } else {
            xhr.send();
        }

        // Return JSON with status and response
        var result = JSON.stringify({
            status: xhr.status,
            response: xhr.responseText
        });

        var lengthBytes = lengthBytesUTF8(result) + 1;
        var stringOnWasmHeap = _malloc(lengthBytes);
        stringToUTF8(result, stringOnWasmHeap, lengthBytes);
        return stringOnWasmHeap;
    } catch (e) {
        var errorResult = JSON.stringify({
            status: 0,
            response: "",
            error: e.toString()
        });
        var lengthBytes = lengthBytesUTF8(errorResult) + 1;
        var stringOnWasmHeap = _malloc(lengthBytes);
        stringToUTF8(errorResult, stringOnWasmHeap, lengthBytes);
        return stringOnWasmHeap;
    }
});

// C++ wrapper function
extern "C" char* wasm_http_request(const char* method, const char* url, const char* body, const char* headers_json) {
    return wasm_http_request_impl(method, url, body, headers_json);
}

#endif  // __EMSCRIPTEN__
