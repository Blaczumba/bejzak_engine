add_library(CommonCamera camera.h   
	projection.h  orthographic_projection.h perspective_projection.h
	orthographic_projection.cpp perspective_projection.cpp
	camera_impl.h camera_impl.cpp camera_controller.h
)
target_link_libraries(CommonCamera PUBLIC CommonWindow)

target_include_directories(CommonCamera PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonCamera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})