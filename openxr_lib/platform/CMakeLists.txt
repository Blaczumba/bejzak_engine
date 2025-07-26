add_library(OpenxrPlatform android_platform.cpp)

target_link_libraries(OpenxrPlatform PUBLIC OpenXR::headers)

target_include_directories(OpenxrPlatform PUBLIC ${PROJECT_SOURCE_DIR}/openxr ${PROJECT_SOURCE_DIR}/sources)
target_include_directories(OpenxrPlatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
