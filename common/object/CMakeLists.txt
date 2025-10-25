add_library(CommonObject object.h object.cpp)

target_link_libraries(CommonObject CommonECSEntity)

target_include_directories(CommonObject PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonObject PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
