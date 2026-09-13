add_library(CommonUtil geometry.cpp buffer_manip.cpp
    buffer_manip.h types.h geometry.h primitives.h asset_manager.h engine_exception.h resource_handles.h ref.h ref_table.h)

target_link_libraries(CommonUtil PUBLIC CommonImageLoader)
target_link_libraries(CommonUtil PUBLIC glm::glm)

target_include_directories(CommonUtil PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})