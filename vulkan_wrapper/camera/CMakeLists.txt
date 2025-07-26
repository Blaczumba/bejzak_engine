add_library(Camera fps_camera.cpp)

target_link_libraries(Camera PUBLIC CommonWindow)

target_include_directories(Camera PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})