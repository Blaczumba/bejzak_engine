add_library(OpenxrSwapchain swapchain.h swapchain.cpp)

target_link_libraries(OpenxrSwapchain PRIVATE OpenXR::headers)
target_link_libraries(OpenxrSwapchain PRIVATE OpenxrSession OpenxrGraphicsPlugin)

target_include_directories(OpenxrSwapchain PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrSwapchain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
