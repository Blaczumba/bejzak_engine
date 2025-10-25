add_library(PhysicalDevice physical_device.h physical_device.cpp)

target_link_libraries(PhysicalDevice PUBLIC LogicalDevice Instance)

target_include_directories(PhysicalDevice PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(PhysicalDevice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})