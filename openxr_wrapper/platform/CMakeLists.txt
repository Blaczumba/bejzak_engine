add_library(OpenxrPlatform INTERFACE platform.h)

if (ANDROID)
	add_library(OpenxrPlatformAndroid android_platform.h android_platform.cpp)
	target_compile_definitions(OpenxrPlatformAndroid PUBLIC XR_USE_PLATFORM_ANDROID)
	target_link_libraries(OpenxrPlatformAndroid PUBLIC OpenXR::headers NativeAppGlue android spdlog)
	target_link_libraries(OpenxrPlatformAndroid PRIVATE OpenxrGraphicsPlugin) # TODO: this theoretically is not needed but it looks like it demads it when linking.
	
	target_link_libraries(OpenxrPlatform INTERFACE OpenxrPlatformAndroid)
endif()

target_link_libraries(OpenxrPlatform INTERFACE OpenXR::headers)

target_include_directories(OpenxrPlatform INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrPlatform INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
