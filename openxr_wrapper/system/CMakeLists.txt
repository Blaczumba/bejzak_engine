add_library(OpenxrSystem system.h system.cpp)

target_link_libraries(OpenxrSystem PRIVATE OpenXR::headers)
target_link_libraries(OpenxrSystem PRIVATE OpenxrInstance)

target_include_directories(OpenxrSystem PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
