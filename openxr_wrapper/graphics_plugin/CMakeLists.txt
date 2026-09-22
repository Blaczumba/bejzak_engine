add_library(OpenxrGraphicsPlugin INTERFACE graphics_plugin.h)

if (Vulkan_FOUND)
    # TODO: for VR headset it is necessary (refactor it to accept any SDK version)
    target_compile_definitions(MemoryAllocator PRIVATE VMA_VULKAN_VERSION=1002000)

    target_compile_definitions(OpenxrGraphicsPlugin INTERFACE XR_USE_GRAPHICS_API_VULKAN)

    add_library(OpenxrGraphicsPluginVulkan graphics_plugin_vulkan.h graphics_plugin_vulkan.cpp)
    target_link_libraries(OpenxrGraphicsPluginVulkan PUBLIC Vulkan::Vulkan OpenXR::headers spdlog)
    target_link_libraries(OpenxrGraphicsPluginVulkan PUBLIC VulkanLayer)

    target_include_directories(OpenxrGraphicsPluginVulkan PUBLIC ${PROJECT_SOURCE_DIR})
    target_include_directories(OpenxrGraphicsPluginVulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_compile_definitions(OpenxrGraphicsPluginVulkan PUBLIC XR_USE_GRAPHICS_API_VULKAN)

    target_link_libraries(OpenxrGraphicsPlugin INTERFACE OpenxrGraphicsPluginVulkan)
endif ()

target_link_libraries(OpenxrGraphicsPlugin INTERFACE CommonAbstractions PresentationGraphicsCommunication)

target_include_directories(OpenxrGraphicsPlugin INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrGraphicsPlugin INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})