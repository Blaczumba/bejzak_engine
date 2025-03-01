add_library(Attachment attachment_layout.cpp)

target_link_libraries(Attachment PUBLIC Vulkan::Vulkan)

target_include_directories(Attachment PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Attachment PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})