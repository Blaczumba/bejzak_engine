add_library(CommonMath engine_math.h engine_math.cpp)

target_link_libraries(CommonMath PUBLIC glm::glm)

target_include_directories(CommonMath PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})