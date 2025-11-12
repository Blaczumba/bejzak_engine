add_library(CommonUtil types.h geometry.h geometry.cpp primitives.h vertex_builder.h vertex_builder.cpp  asset_manager.h index_buffer.h "index_buffer.cpp")

target_include_directories(CommonUtil PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})