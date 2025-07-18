include_directories(${CMAKE_SOURCE_DIR}/external/glm)
include_directories(${CMAKE_SOURCE_DIR}/external/)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_subdirectory(lib)

add_subdirectory(window)
add_subdirectory(instance)
add_subdirectory(surface)
add_subdirectory(debug_messenger)
add_subdirectory(physical_device)
add_subdirectory(logical_device)
add_subdirectory(memory_allocator)
add_subdirectory(memory_objects)
add_subdirectory(render_pass)
add_subdirectory(swapchain)
add_subdirectory(command_buffer)
add_subdirectory(pipeline)
add_subdirectory(model_loader)
add_subdirectory(descriptor_set)
add_subdirectory(camera)
add_subdirectory(screenshot)
add_subdirectory(entity_component_system)
add_subdirectory(object)
add_subdirectory(primitives)
add_subdirectory(scene)
add_subdirectory(thread_pool)
add_subdirectory(framebuffer)
add_subdirectory(resource_manager)

add_compile_definitions(SHADERS_PATH="${CMAKE_SOURCE_DIR}/shaders/")
add_compile_definitions(TEXTURES_PATH="${CMAKE_SOURCE_DIR}/assets/textures/")
add_compile_definitions(MODELS_PATH="${CMAKE_SOURCE_DIR}/assets/models/")

add_subdirectory(application)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

if(WIN32)
    set(SHADER_COMPILE_SCRIPT
    "${CMAKE_SOURCE_DIR}/shaders/compile_shaders.bat")
elseif(UNIX)
    set(SHADER_COMPILE_SCRIPT
    "${CMAKE_SOURCE_DIR}/shaders/compile_shaders.sh")
endif()

file(CHMOD ${SHADER_COMPILE_SCRIPT} PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

add_custom_target(
    CompileShaders
    COMMAND ${SHADER_COMPILE_SCRIPT}
    COMMENT "Compiling shaders...")

add_dependencies(${CMAKE_PROJECT_NAME} CompileShaders)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Application)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/external/tinygltf")

add_subdirectory(tests)
