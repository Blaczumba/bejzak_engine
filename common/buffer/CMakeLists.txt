add_library(CommonBuffer buffer_lib.h buffer_lib.cpp index_buffer_lib.h index_buffer_lib.cpp vertex_buffer_lib.h vertex_buffer_lib.cpp)

target_include_directories(CommonBuffer PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})