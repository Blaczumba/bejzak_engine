add_library(Primitives geometry.cpp primitives.h vertex_builder.cpp)

target_include_directories(Primitives PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Primitives PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})