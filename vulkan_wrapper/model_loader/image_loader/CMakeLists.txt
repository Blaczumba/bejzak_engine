set(KTX_DIR ${PROJECT_SOURCE_DIR}/external/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c)


add_library(ImageLoader image_loader.h image_loader.cpp ${KTX_SOURCES})

target_link_libraries(ImageLoader PUBLIC Vulkan::Vulkan)
target_link_libraries(ImageLoader PUBLIC LogicalDevice CommandBuffer)

target_include_directories(ImageLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(ImageLoader PUBLIC ${PROJECT_SOURCE_DIR}/external/ktx/include)
target_include_directories(ImageLoader PUBLIC ${PROJECT_SOURCE_DIR}/external/ktx/other_include)
target_include_directories(ImageLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})