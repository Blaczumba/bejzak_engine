add_library(LibStrongTypes strong_int.cpp)

target_include_directories(LibStrongTypes PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(LibStrongTypes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})