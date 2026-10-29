add_library(Engine main_loop.h main_loop.cpp)

target_link_libraries(Engine PRIVATE CommonAbstractions)

target_include_directories(Engine PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(Engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})