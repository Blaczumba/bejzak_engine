add_library(CommonWindow INTERFACE window.h)

target_link_libraries(CommonWindow INTERFACE CommonInputManager)

if((WIN32 OR UNIX) AND NOT ANDROID)
	add_library(CommonWindowGLFW window_glfw.h window_glfw.cpp)
	target_link_libraries(CommonWindowGLFW PUBLIC glfw)
	target_include_directories(CommonWindowGLFW PUBLIC ${PROJECT_SOURCE_DIR})
	target_include_directories(CommonWindowGLFW PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

	target_link_libraries(CommonWindow INTERFACE CommonWindowGLFW)
endif()

target_include_directories(CommonWindow INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(CommonWindow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
