add_library(CommonECSRegistry registry.cpp)

target_include_directories(CommonECSRegistry PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonECSRegistry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})