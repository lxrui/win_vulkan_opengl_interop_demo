cmake_minimum_required(VERSION 3.10)
project(vulkan_opengl_interop_demo)

set(glfw3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw3.3.8/lib/cmake/glfw3)
find_package(glfw3 REQUIRED)

add_subdirectory(Vulkan-headers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glad/include)

add_executable(${PROJECT_NAME}
    main.cc
    vulkan_resources.h
    vulkan_pfn.h
    vulkan_pfn.cc
    vulkan_context.h
    vulkan_context.cc
    vulkan_command_buffer.h
    vulkan_command_buffer.cc
    vulkan_pipeline.h
    vulkan_pipeline.cc
    vulkan_texture.h
    vulkan_texture.cc
    glad/src/glad.c
)

target_link_libraries(${PROJECT_NAME} PUBLIC vulkan-headers glfw)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DVK_USE_PLATFORM_WIN32_KHR -DVK_NO_PROTOTYPES=1)

#copy image file to run path
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/1.jpg
    ${CMAKE_CURRENT_BINARY_DIR}/Release/1.jpg
    COPYONLY
)