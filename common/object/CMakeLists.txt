add_library(CommonObject object.h object.cpp)

target_link_libraries(CommonObject PUBLIC CommonECSEntity)
target_link_libraries(CommonObject PRIVATE glm::glm)

target_include_directories(CommonObject PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonObject PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
