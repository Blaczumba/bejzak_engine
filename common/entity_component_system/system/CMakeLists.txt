add_library(CommonECSSystem movement_system.cpp)

target_include_directories(CommonECSSystem PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonECSSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})