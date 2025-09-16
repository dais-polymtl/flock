add_subdirectory(scalar)
add_subdirectory(aggregate)

set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/input_parser.cpp
    PARENT_SCOPE)
