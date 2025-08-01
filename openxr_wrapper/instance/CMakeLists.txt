add_library(OpenxrInstance instance.h instance.cpp)

target_link_libraries(OpenxrInstance PRIVATE OpenXR::headers)
target_link_libraries(OpenxrInstance PRIVATE OpenxrPlatform OpenxrGraphicsPlugin)

target_include_directories(OpenxrInstance PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrInstance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
