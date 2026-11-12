add_library(Engine simulation.h simulation.cpp system.h system.cpp input_manager.h input_manager.cpp frame_composer.h frame_composer_default.h frame_composer_default.cpp timer.h timer.cpp)

target_link_libraries(Engine PRIVATE CommonAbstractions)

target_include_directories(Engine PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(Engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})