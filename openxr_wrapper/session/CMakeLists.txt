add_library(OpenxrSession session.h session.cpp)

target_link_libraries(OpenxrSession PRIVATE OpenXR::headers)
target_link_libraries(OpenxrSession PRIVATE OpenxrSystem)

target_include_directories(OpenxrSession PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrSession PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
