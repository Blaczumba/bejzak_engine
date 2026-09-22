add_library(VulkanWrapperSynchronization fence.h fence.cpp semaphore.h semaphore.cpp)

target_link_libraries(VulkanWrapperSynchronization PUBLIC Vulkan::Vulkan)

target_include_directories(VulkanWrapperSynchronization PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(VulkanWrapperSynchronization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})