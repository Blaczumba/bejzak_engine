add_library(CommonAbstractions asset_manager.h asset_manager.cpp contexts.h graphics_context.h presentation.h)

target_link_libraries(CommonAbstractions PUBLIC glm::glm CommonImageLoader)

target_include_directories(CommonAbstractions PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonAbstractions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})