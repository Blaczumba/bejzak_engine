add_library(OpenxrInputManager xr_input_manager.h xr_input_manager.cpp action.h action.cpp)

target_link_libraries(OpenxrInputManager PRIVATE OpenXR::headers spdlog glm::glm)
target_link_libraries(OpenxrInputManager PRIVATE OpenxrSpace OpexrAction)

target_include_directories(OpenxrInputManager PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(OpenxrInputManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
