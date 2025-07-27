add_library(CommonCamera    
	projection.h  orthographic_projection.h perspective_projection.h
	orthographic_projection.cpp perspective_projection.cpp
	camera.h camera.cpp camera_controller.h
)
target_link_libraries(CommonCamera PUBLIC CommonWindow)

target_include_directories(CommonCamera PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonCamera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})