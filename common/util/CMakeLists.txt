add_library(CommonUtil geometry.cpp buffer_manip.cpp
    buffer_manip.h types.h geometry.h primitives.h asset_manager.h)

target_include_directories(CommonUtil PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})