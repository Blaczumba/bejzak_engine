add_library(WindowGLFW window_glfw.cpp)

target_link_libraries(WindowGLFW PUBLIC glfw Surface)

target_include_directories(WindowGLFW PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(WindowGLFW PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
