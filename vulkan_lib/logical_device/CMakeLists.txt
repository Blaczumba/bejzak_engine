add_library(LogicalDevice logical_device.cpp)

target_link_libraries(LogicalDevice PUBLIC PhysicalDevice PRIVATE MemoryAllocator)

target_include_directories(LogicalDevice PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(LogicalDevice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
