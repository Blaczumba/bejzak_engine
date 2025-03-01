add_library(LibSparseSet sparse_set.cpp)

target_include_directories(LibSparseSet PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(LibSparseSet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})