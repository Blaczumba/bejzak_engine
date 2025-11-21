cmake_minimum_required(VERSION 3.21)

set(ENGINE_NAME BejzakEngine)

project(${ENGINE_NAME})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

# if(CLANG_TIDY_EXE)
#     message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
#     set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
# endif()

find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
endif()

# find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
# 
# if(NOT Boost_FOUND)
#     message(WARNING "Boost not found!")
# else()
#     add_definitions(-DBOOST_ENABLED)
# 
#     set(Boost_USE_STATIC_LIBS ON)
#     set(Boost_USE_MULTITHREADED ON)
#     set(Boost_USE_STATIC_RUNTIME OFF)
# 
#     set(BOOST_COMPONENTS filesystem system)
# endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building in Release mode.")
else()
    message(STATUS "Building in Debug mode.")
    add_definitions(-DVALIDATION_LAYERS_ENABLED)
endif()

add_definitions(-DGLM_FORCE_NO_CTOR_INIT)

if(NOT ANDROID)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    add_subdirectory(${PROJECT_SOURCE_DIR}/external/glfw)
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/external/tinyobjloader)
add_subdirectory(${PROJECT_SOURCE_DIR}/external/googletest)

if(WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
elseif(UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -pedantic -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

include_directories(${PROJECT_SOURCE_DIR}/external/glm)
include_directories(${PROJECT_SOURCE_DIR}/external/)

add_library(${ENGINE_NAME} INTERFACE)

add_subdirectory(lib)

add_subdirectory(common)
set(COMMON_ENGINE_NAME ${ENGINE_NAME}COMMON)
add_library(${COMMON_ENGINE_NAME} INTERFACE)
target_include_directories(${COMMON_ENGINE_NAME} INTERFACE
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(${COMMON_ENGINE_NAME} INTERFACE
    CommonCamera CommonInputManager CommonECS
    CommonObject CommonUtil
    CommonScene CommonStandardFileLoader
    CommonModelLoader
)
target_link_libraries(${ENGINE_NAME} INTERFACE ${COMMON_ENGINE_NAME})

if(Vulkan_FOUND)
    add_subdirectory(vulkan)
    set(VULKAN_ENGINE_NAME ${ENGINE_NAME}Vulkan)
    add_library(${VULKAN_ENGINE_NAME} INTERFACE)
    target_link_libraries(${VULKAN_ENGINE_NAME} INTERFACE
            Instance AssetManager
            Surface DebugMessenger
            PhysicalDevice MemoryAllocator
            LogicalDevice Renderpass
            Swapchain Buffer CommandBuffer
            Framebuffer Pipeline
            DescriptorSet
    )
    target_link_libraries(${ENGINE_NAME} INTERFACE ${VULKAN_ENGINE_NAME})
endif()

if (ANDROID)
    add_subdirectory(openxr_wrapper)
    # target_compile_definitions(${ENGINE_NAME} INTERFACE XR_USE_PLATFORM_ANDROID)
    target_link_libraries(${ENGINE_NAME} INTERFACE
            OpenxrGraphicsPlugin OpenxrPlatform OpenxrInstance
            OpenxrSystem OpenxrSession OpenxrSwapchain
            OpenxrSpace

            OpenXR::headers

            CommonAndroidFileLoader
    )
endif ()
