add_library(OpenxrSpace space.h space.cpp)

target_link_libraries(OpenxrSpace PRIVATE OpenXR::headers)

target_include_directories(OpenxrSpace PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrSpace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
