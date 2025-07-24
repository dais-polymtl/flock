find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
list(REMOVE_ITEM TEST_SOURCES "test_main.cpp")

file(COPY flockmtl_test.db DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}_tests test_main.cpp ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_extension
                                                    GTest::gtest GTest::gmock)

add_test(AllTestsInMain ${PROJECT_NAME}_tests)
