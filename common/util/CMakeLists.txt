add_library(CommonPrimitives types.h geometry.h geometry.cpp primitives.h vertex_builder.h vertex_builder.cpp index_buffer_index.h)

target_include_directories(CommonPrimitives PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonPrimitives PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})