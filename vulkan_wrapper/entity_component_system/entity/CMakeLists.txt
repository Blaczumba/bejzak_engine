add_library(Entity entity_manager.cpp)

target_include_directories(Entity PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Entity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
