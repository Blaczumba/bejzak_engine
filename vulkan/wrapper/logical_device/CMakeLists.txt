add_library(LogicalDevice logical_device.h logical_device.cpp
        extensions_connector.h extensions_connector.cpp resource_destroyer.h resource_destroyer.cpp modifiers.h modifiers.cpp "optional_extended_features.h")

target_link_libraries(LogicalDevice PUBLIC PhysicalDevice MemoryAllocator)

target_include_directories(LogicalDevice PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(LogicalDevice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
