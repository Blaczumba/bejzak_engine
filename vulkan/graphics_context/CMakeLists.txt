add_library(VulkanLayer graphics_context.h presentation.h presentation.cpp presentation_lib.h presentation_lib.cpp graphics_context.cpp graphics_context_lib.h graphics_context_lib.cpp)

target_link_libraries(VulkanLayer PUBLIC Vulkan::Vulkan)
target_link_libraries(VulkanLayer PRIVATE glm::glm)
target_link_libraries(VulkanLayer PUBLIC CommonModelLoader Instance PhysicalDevice LogicalDevice Surface Swapchain PresentationGraphicsCommunication)

target_include_directories(VulkanLayer PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(VulkanLayer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
