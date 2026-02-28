#pragma once

#ifdef __EMSCRIPTEN__

extern "C" char* wasm_http_request(const char* method, const char* url, const char* body, const char* headers_json);

#endif  // __EMSCRIPTEN__
