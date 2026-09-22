add_library(CommonUtil geometry.cpp
    types.h geometry.h primitives.h engine_exception.h resource_handles.h)

target_link_libraries(CommonUtil PUBLIC CommonImageLoader)
target_link_libraries(CommonUtil PUBLIC glm::glm)

target_include_directories(CommonUtil PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})