add_library(CommonInputManager INTERFACE mouse_keyboard_manager.h)

if((WIN32 OR UNIX) AND NOT ANDROID)
	add_library(CommonInputManagerGlfw mouse_keyboard_manager_glfw.h mouse_keyboard_manager_glfw.cpp)
	target_link_libraries(CommonInputManagerGlfw PUBLIC glfw)
	target_link_libraries(CommonInputManagerGlfw PUBLIC CommonWindowGLFW)
	target_include_directories(CommonInputManagerGlfw PUBLIC ${PROJECT_SOURCE_DIR})
	target_include_directories(CommonInputManagerGlfw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

	target_link_libraries(CommonInputManager INTERFACE CommonInputManagerGlfw)
endif()

target_include_directories(CommonInputManager INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(CommonInputManager INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
