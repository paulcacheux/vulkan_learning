#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <vector>

#include "game.hpp"
#include "scene.hpp"
#include "vulkan/instance.hpp"
#include "window.hpp"

const std::size_t WIDTH = 800;
const std::size_t HEIGHT = 600;

int main() {
    try {
        app::WindowContext context;
        app::Window window(WIDTH, HEIGHT, "Vulkan window");

        scene::Scene scene;
        scene::Camera camera;
        Game game(scene, camera);

        vulkan::Instance instance(window, game);

        window.linkToInstance(&instance);
        window.setupResizeCallback();

        auto lastTime = std::chrono::high_resolution_clock::now();
        while (!window.shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float dt
                = std::chrono::duration<float, std::chrono::seconds::period>(
                      currentTime - lastTime)
                      .count();
            lastTime = currentTime;

            context.pollEvents();
            game.update(dt);
            instance.drawFrame();
        }
        instance.deviceWaitIdle();

    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
