add_library(Swapchain swapchain.h swapchain.cpp)

target_link_libraries(Swapchain PUBLIC LogicalDevice)

target_include_directories(Swapchain PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Swapchain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
