add_library(CommonUtil types.h geometry.h geometry.cpp primitives.h vertex_builder.h vertex_builder.cpp index_buffer_index.h asset_manager.h)

target_include_directories(CommonUtil PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})