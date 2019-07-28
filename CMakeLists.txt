cmake_minimum_required(VERSION 3.9.1)

project(VulkanTuto)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O")

file(GLOB_RECURSE src_files "src/*.cpp" "vma/*.cpp")
include_directories(vma)
include_directories(include)

add_executable(vulkan_tuto ${src_files})

find_package(glm CONFIG REQUIRED)
target_link_libraries(vulkan_tuto PRIVATE glm)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vulkan_tuto PRIVATE glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(vulkan_tuto PRIVATE Vulkan::Vulkan)

# shaders
if (WIN32)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
      set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    else()
      set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
    endif()
else()
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
endif()

file(GLOB_RECURSE shaders_files "shaders/*.frag" "shaders/*.vert")

foreach (GLSL ${shaders_files})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV_DIR "${PROJECT_BINARY_DIR}/shaders")
    set(SPIRV "${SPIRV_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIRV_DIR}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(vulkan_tuto shaders)

add_custom_command(TARGET vulkan_tuto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan_tuto>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:vulkan_tuto>/shaders"
)
