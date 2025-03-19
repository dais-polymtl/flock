add_subdirectory(llm_complete)
add_subdirectory(llm_complete_json)
add_subdirectory(llm_filter)
add_subdirectory(fusion_combanz)
add_subdirectory(fusion_combmnz)
add_subdirectory(fusion_combsum)
add_subdirectory(fusion_rrf)
add_subdirectory(llm_embedding)

set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/scalar.cpp
    PARENT_SCOPE)
