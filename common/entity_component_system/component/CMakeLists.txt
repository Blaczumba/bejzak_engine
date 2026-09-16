add_library(CommonECSComponent component_pool.cpp velocity.h position.h transform.h position.h material.h)

target_link_libraries(CommonECSComponent PUBLIC glm::glm)
target_link_libraries(CommonECSComponent PUBLIC CommonUtil CommonRef)

target_include_directories(CommonECSComponent PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonECSComponent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})