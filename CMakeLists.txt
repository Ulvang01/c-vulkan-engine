cmake_minimum_required(VERSION 3.10)
project(c_vulkan_engine C)

set(CMAKE_C_STANDARD 11)

find_package(Vulkan REQUIRED)

file(GLOB SOURCES "src/*.c")
add_executable(c_vulkan_engine ${SOURCES})

target_link_libraries(c_vulkan_engine
    Vulkan::Vulkan
    glfw3
)

target_include_directories(c_vulkan_engine PRIVATE
    "${CMAKE_SOURCE_DIR}/external/glfw/include"
)

target_link_directories(c_vulkan_engine PRIVATE
    "${CMAKE_SOURCE_DIR}/external/glfw/build/src"
)
