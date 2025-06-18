find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp")

# Exclude specific llm files in the scalar folder (but keep llm_embedding and
# llm_filter)
file(GLOB LLM_SCALAR_FILES_TO_EXCLUDE
     "${CMAKE_CURRENT_SOURCE_DIR}/functions/scalar/llm_complete.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/functions/scalar/llm_complete_json.cpp")
if(LLM_SCALAR_FILES_TO_EXCLUDE)
  list(REMOVE_ITEM TEST_SOURCES ${LLM_SCALAR_FILES_TO_EXCLUDE})
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/flockmtl_test.db
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}_tests test_main.cpp ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_extension
                                                    GTest::gtest GTest::gmock)

add_test(AllTestsInMain ${PROJECT_NAME}_tests)
