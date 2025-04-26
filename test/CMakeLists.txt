find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

target_link_libraries(
  ${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_extension GTest::gtest
                                GTest::gtest_main GTest::gmock)

include(GoogleTest)
gtest_discover_tests(flockmtl_tests)
