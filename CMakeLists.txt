cmake_minimum_required(VERSION 3.10)
project(BoogerAids C)

set(CMAKE_C_STANDARD 11)

find_package(Vulkan REQUIRED)

add_executable(BoogerAids main.c)

target_link_libraries(BoogerAids
    Vulkan::Vulkan
    glfw3
)

target_include_directories(BoogerAids PRIVATE
    "${CMAKE_SOURCE_DIR}/external/glfw/include"
)

target_link_directories(BoogerAids PRIVATE
    "${CMAKE_SOURCE_DIR}/external/glfw/build/src"
)
