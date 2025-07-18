add_library(DebugMessenger debug_messenger.cpp debug_messenger_utils.cpp)

target_link_libraries(DebugMessenger PUBLIC Instance)

target_include_directories(DebugMessenger PUBLIC ${PROJECT_SOURCE_DIR}/sources/instance)
target_include_directories(DebugMessenger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
