add_library(ECSRegistry registry.cpp)

target_include_directories(ECSRegistry PUBLIC ${PROJECT_SOURCE_DIR}/sources)
target_include_directories(ECSRegistry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})