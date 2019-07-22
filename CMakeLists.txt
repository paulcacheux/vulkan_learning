cmake_minimum_required(VERSION 3.9.1)

project(VulkanTuto)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O")

file(GLOB_RECURSE src_files "src/*.cpp")

add_executable(vulkan_tuto ${src_files})

find_package(glm CONFIG REQUIRED)
target_link_libraries(vulkan_tuto PRIVATE glm)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vulkan_tuto PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(vulkan_tuto PRIVATE Vulkan::Vulkan)
