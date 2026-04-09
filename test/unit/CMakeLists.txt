find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
list(REMOVE_ITEM TEST_SOURCES "test_main.cpp")

file(COPY unit_test.db DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}_tests test_main.cpp ${TEST_SOURCES})

set(FLOCK_TEST_LINK_LIBS
    ${PROJECT_NAME}_extension
    GTest::gtest
    GTest::gmock
    duckdb_static
    duckdb_generated_extension_loader
    core_functions_extension
    parquet_extension
    json_extension)

target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${FLOCK_TEST_LINK_LIBS})

add_test(AllTestsInMain ${PROJECT_NAME}_tests)
