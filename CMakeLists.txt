cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set extension name here
set(TARGET_NAME flock)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${TARGET_NAME})

# Include directories
include_directories(src/include)
add_subdirectory(src)

# Find dependencies
if(NOT EMSCRIPTEN)
    find_package(CURL REQUIRED)
endif()
find_package(nlohmann_json CONFIG REQUIRED)

# Build the DuckDB static and loadable extensions
build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

# Link the filesystem library for GCC < 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION
                                            VERSION_LESS 9.0)
  target_link_libraries(${EXTENSION_NAME} -lstdc++fs)
endif()

# Check if we're in debug mode and enable AddressSanitizer
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Enabling AddressSanitizer for Debug build")
  # Enable AddressSanitizer
  target_compile_options(${EXTENSION_NAME} PRIVATE -fsanitize=address
                                                   -fno-omit-frame-pointer)
  target_link_options(${EXTENSION_NAME} PRIVATE -fsanitize=address)

  target_compile_options(${LOADABLE_EXTENSION_NAME}
                         PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(${LOADABLE_EXTENSION_NAME} PRIVATE -fsanitize=address)
endif()

# Link libraries for the static extension
if(NOT EMSCRIPTEN)
    target_link_libraries(${EXTENSION_NAME} CURL::libcurl)
endif()
target_link_libraries(${EXTENSION_NAME} nlohmann_json::nlohmann_json)

# Link libraries for the loadable extension
if(NOT EMSCRIPTEN)
    target_link_libraries(${LOADABLE_EXTENSION_NAME} CURL::libcurl)
endif()
target_link_libraries(${LOADABLE_EXTENSION_NAME} nlohmann_json::nlohmann_json)

# WASM builds use EM_JS with synchronous XMLHttpRequest for HTTP

# Install the extension
install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  message(STATUS "Enabling code coverage for Debug build")
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

# # Add the test directory
# enable_testing()
# add_subdirectory(test/unit)
