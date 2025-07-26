add_library(Object object.cpp)

target_link_libraries(Object DescriptorSet Scene Entity)

target_include_directories(Object PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Object PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
