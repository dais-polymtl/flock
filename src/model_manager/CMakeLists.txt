add_subdirectory(providers/adapters)

set(EXTENSION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/adapters/anthropic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/adapters/azure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/adapters/openai.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/adapters/ollama.cpp
    ${EXTENSION_SOURCES}
    PARENT_SCOPE)
