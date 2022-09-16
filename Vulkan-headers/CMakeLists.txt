cmake_minimum_required(VERSION 3.6)
project(vulkan-headers)

add_library(${PROJECT_NAME} INTERFACE)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
    INTERFACE_LINK_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib
)