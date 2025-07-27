add_library(CommonECSEntity entity_manager.cpp)

target_include_directories(CommonECSEntity PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonECSEntity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
