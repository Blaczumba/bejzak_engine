add_library(CommonNetworking socket.h socket.cpp)

if(WIN32)
    target_link_libraries(CommonNetworking PRIVATE ws2_32)
endif()

target_include_directories(CommonNetworking PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonNetworking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
