add_library(CommonBuffer buffer_utils.h buffer_utils.cpp index_buffer_utils.h index_buffer_utils.cpp vertex_buffer_utils.h vertex_buffer_utils.cpp)

target_include_directories(CommonBuffer PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})