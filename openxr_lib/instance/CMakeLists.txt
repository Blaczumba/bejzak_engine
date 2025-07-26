add_library(OpenxrInstance instance.cpp)

target_link_libraries(OpenxrInstance PUBLIC OpenXR::headers)
target_link_libraries(OpenxrInstance PRIVATE OpenxrPlatform)

target_include_directories(OpenxrInstance PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrInstance PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
