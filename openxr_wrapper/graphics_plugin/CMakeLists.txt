add_library(OpenxrGraphicsPlugin INTERFACE graphics_plugin.h)

if (Vulkan_FOUND)
    add_library(OpenxrGraphicsPluginVulkan graphics_plugin_vulkan.h graphics_plugin_vulkan.cpp)
    target_link_libraries(OpenxrGraphicsPluginVulkan PUBLIC Vulkan::Vulkan OpenXR::headers)
    target_link_libraries(OpenxrGraphicsPluginVulkan PUBLIC Instance DebugMessenger LogicalDevice)

    target_include_directories(OpenxrGraphicsPluginVulkan PRIVATE ${PROJECT_SOURCE_DIR})
    target_include_directories(OpenxrGraphicsPluginVulkan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_compile_definitions(OpenxrGraphicsPluginVulkan PRIVATE XR_USE_GRAPHICS_API_VULKAN)

    target_link_libraries(OpenxrGraphicsPlugin INTERFACE OpenxrGraphicsPluginVulkan)
endif ()

target_include_directories(OpenxrGraphicsPlugin INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrGraphicsPlugin INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})