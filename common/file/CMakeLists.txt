add_library(CommonStandardFileLoader file_loader.h standard_file_loader.h standard_file_loader.cpp)

target_include_directories(CommonStandardFileLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonStandardFileLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (ANDROID)
    add_library(CommonAndroidFileLoader file_loader.h android_file_loader.h android_file_loader.cpp)

    target_include_directories(CommonAndroidFileLoader PUBLIC ${PROJECT_SOURCE_DIR})
    target_include_directories(CommonAndroidFileLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif ()