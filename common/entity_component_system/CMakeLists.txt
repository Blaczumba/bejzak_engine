add_subdirectory(component)
add_subdirectory(entity)
add_subdirectory(system)
add_subdirectory(registry)

add_library(CommonECS INTERFACE)
target_link_libraries(CommonECS INTERFACE CommonECSRegistry CommonECSSystem CommonECSComponent CommonECSEntity)

target_include_directories(CommonECS INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(CommonECS INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
