add_library(Status status.cpp)

target_link_libraries(Status Vulkan::Vulkan)

target_include_directories(Status PUBLIC ${PROJECT_SOURCE_DIR}/sources)
target_include_directories(Status PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
