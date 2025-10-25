add_library(OpenxrPlatform platform.h android_platform.h android_platform.cpp)

target_link_libraries(OpenxrPlatform PRIVATE OpenXR::headers)

target_include_directories(OpenxrPlatform PRIVATE ${PROJECT_SOURCE_DIR}/openxr ${PROJECT_SOURCE_DIR}/sources)
target_include_directories(OpenxrPlatform PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
