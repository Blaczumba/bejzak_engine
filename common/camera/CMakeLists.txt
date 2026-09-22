add_library(CommonCamera    
	projection.h projection.cpp camera.h camera.cpp camera_controller.h
)

target_link_libraries(CommonCamera PUBLIC CommonWindow CommonInputManager)
target_link_libraries(CommonCamera PRIVATE glm::glm)

target_include_directories(CommonCamera PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonCamera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})