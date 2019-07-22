#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <exception>
#include <iostream>
#include <vector>

#include "vulkan.hpp"
#include "window.hpp"

const std::size_t WIDTH = 800;
const std::size_t HEIGHT = 600;

int main() {
    try {
        app::WindowContext context;
        app::Window window(WIDTH, HEIGHT, "Vulkan window");
        app::VulkanInstance instance;

        // instance.listExtensions();

        while (!window.shouldClose()) {
            context.pollEvents();
        }
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
