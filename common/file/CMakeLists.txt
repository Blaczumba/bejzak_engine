add_library(CommonFileLoader INTERFACE file_loader.h)

if (ANDROID)
    add_library(CommonAndroidFileLoader file_loader.h android_file_loader.h android_file_loader.cpp)
    target_link_libraries(CommonAndroidFileLoader PUBLIC NativeAppGlue android)

    target_include_directories(CommonAndroidFileLoader PUBLIC ${PROJECT_SOURCE_DIR})
    target_include_directories(CommonAndroidFileLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(CommonFileLoader INTERFACE CommonAndroidFileLoader)
else()
    add_library(CommonStandardFileLoader standard_file_loader.h standard_file_loader.cpp file.h file.cpp)

    target_include_directories(CommonStandardFileLoader PUBLIC ${PROJECT_SOURCE_DIR})
    target_include_directories(CommonStandardFileLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(CommonFileLoader INTERFACE CommonStandardFileLoader)
endif()

target_include_directories(CommonFileLoader INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(CommonFileLoader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})