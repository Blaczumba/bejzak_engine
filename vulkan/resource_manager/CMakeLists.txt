add_library(AssetManager asset_manager.h asset_manager.cpp pipeline_manager.h pipeline_manager.cpp 
hasher.h 
bindless_descriptor_set_writer.h bindless_descriptor_set_writer.cpp util.h sampler_manager.h sampler_manager.cpp
framebuffer_attachments_manager.h framebuffer_attachments_manager.cpp handle.h
ref.h reference_counter.h reference_counter_with_metadata.h buffer_manager.h buffer_manager.cpp image_manager.h image_manager.cpp resource_manager_allocation_strategy.h)

target_link_libraries(AssetManager PUBLIC Vulkan::Vulkan glm::glm)
target_link_libraries(AssetManager PUBLIC CommonModelLoader CommonRef LogicalDevice MemoryObjects Sampler CommonUtil VulkanWrapperUtil)

target_include_directories(AssetManager PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(AssetManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# TODO: refactor
if (ANDROID)
    target_compile_definitions(AssetManager PUBLIC SHADERS_PATH="shaders/")
else ()
    target_compile_definitions(AssetManager PUBLIC SHADERS_PATH="${PROJECT_SOURCE_DIR}/shaders/")
endif ()