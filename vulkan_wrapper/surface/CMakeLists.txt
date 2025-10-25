add_library(Surface surface.h surface.cpp)

target_link_libraries(Surface PUBLIC Vulkan::Vulkan Instance)
target_link_libraries(Surface PUBLIC CommonWindow)

target_include_directories(Surface PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Surface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
