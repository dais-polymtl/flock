find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp")

# Exclude mock_provider files
file(GLOB_RECURSE MOCK_PROVIDER_FILES "*mock_provider*")
if(MOCK_PROVIDER_FILES)
  list(REMOVE_ITEM TEST_SOURCES ${MOCK_PROVIDER_FILES})
endif()

# Exclude llm files in the scalar folder
file(GLOB LLM_SCALAR_FILES
     "${CMAKE_CURRENT_SOURCE_DIR}/functions/scalar/llm*.cpp")
if(LLM_SCALAR_FILES)
  list(REMOVE_ITEM TEST_SOURCES ${LLM_SCALAR_FILES})
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/flockmtl_test.db
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}_tests test_main.cpp ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_extension
                                                    GTest::gtest GTest::gmock)

add_test(AllTestsInMain ${PROJECT_NAME}_tests)
