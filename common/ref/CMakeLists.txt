add_library(CommonRef INTERFACE ref.h)

target_include_directories(CommonRef INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(CommonRef INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})