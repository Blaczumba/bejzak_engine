add_library(OpenxrAction action_set.h action_set.cpp action.h action.cpp)

target_link_libraries(OpenxrAction PRIVATE OpenXR::headers)

target_include_directories(OpenxrAction PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrAction PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
