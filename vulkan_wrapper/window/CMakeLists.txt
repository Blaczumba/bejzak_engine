add_library(Window window.cpp)

target_link_libraries(Window PUBLIC Surface CommonInputManagerGlfw)

if((WIN32 OR UNIX) AND NOT ANDROID)
	add_library(WindowGLFW window_glfw.cpp)
	target_link_libraries(WindowGLFW PUBLIC glfw)
	target_include_directories(WindowGLFW PUBLIC ${PROJECT_SOURCE_DIR})
	target_include_directories(WindowGLFW PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

	target_link_libraries(Window PUBLIC WindowGLFW)
endif()

target_include_directories(Window PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
