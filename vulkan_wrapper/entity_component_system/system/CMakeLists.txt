add_library(ComponentSystem movement_system.cpp)

target_include_directories(ComponentSystem PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(ComponentSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})