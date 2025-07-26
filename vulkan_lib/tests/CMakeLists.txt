set(TEST_NAME BejzakEngineTests)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(${TEST_NAME} test_vulkan.cpp)
target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(${TEST_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/glm)

target_include_directories(${TEST_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})