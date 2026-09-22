cmake_minimum_required(VERSION 3.21)
include(FetchContent)
include(CheckIPOSupported)

set(ENGINE_NAME BejzakEngine)

project(${ENGINE_NAME})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

check_ipo_supported(RESULT supported OUTPUT error)

if(supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO is not supported: ${error}")
endif()

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

# if(CLANG_TIDY_EXE)
#     message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
#     set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
# endif()

find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
endif()

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

add_definitions(-DSPDLOG_USE_STD_FORMAT)
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(spdlog)

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
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

if(NOT ANDROID)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
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
    CommonScene CommonFileLoader
    CommonModelLoader CommonMath CommonBuffer
    CommonAbstractions CommonRef
)
target_link_libraries(${ENGINE_NAME} INTERFACE ${COMMON_ENGINE_NAME})

add_subdirectory(presentation_graphics_communication)
target_link_libraries(${ENGINE_NAME} INTERFACE PresentationGraphicsCommunication)

if(Vulkan_FOUND)
    add_subdirectory(vulkan)
    set(VULKAN_ENGINE_NAME ${ENGINE_NAME}Vulkan)
    add_library(${VULKAN_ENGINE_NAME} INTERFACE)
    target_link_libraries(${VULKAN_ENGINE_NAME} INTERFACE
            Instance AssetManager
            Surface DebugMessenger
            PhysicalDevice MemoryAllocator
            LogicalDevice Renderpass
            Swapchain MemoryObjects CommandBuffer
            Framebuffer Pipeline Sampler
            DescriptorSet VulkanWrapperUtil
            VulkanLayer VulkanWrapperSynchronization VulkanWrapperBuilders
    )
    target_link_libraries(${ENGINE_NAME} INTERFACE ${VULKAN_ENGINE_NAME})
endif()

FetchContent_Declare(OpenXR
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
        GIT_TAG release-1.0.34 #must match the meta quest loader OpenXR version
        #https://developer.oculus.com/downloads/package/oculus-openxr-mobile-sdk#current-openxr-version
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(OpenXR)

add_subdirectory(openxr_wrapper)
target_link_libraries(${ENGINE_NAME} INTERFACE
        OpenxrGraphicsPlugin OpenxrPlatform OpenxrInstance
        OpenxrSystem OpenxrSession OpenxrSwapchain
        OpenxrSpace OpenxrPresentation
        OpenXR::headers
)