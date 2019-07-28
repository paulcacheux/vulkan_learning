#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include <vector>

#include "game.hpp"
#include "scene.hpp"
#include "vulkan/renderer.hpp"
#include "window.hpp"

const std::size_t WIDTH = 800;
const std::size_t HEIGHT = 600;

int main() {
    try {
        app::WindowContext context;
        app::Window window(WIDTH, HEIGHT, "Vulkan window");

        Game game;
        vulkan::Renderer renderer(window, &game.getScene());

        app::GameRendererCoupler coupler{game, renderer};
        window.linkToCoupler(&coupler);
        window.switchToRawMouseMode();

        auto maxFps = 200;
        auto frameMinDuration
            = std::chrono::duration<float, std::chrono::seconds::period>(
                1 / (float)maxFps);

        auto lastTime = std::chrono::high_resolution_clock::now();
        while (!window.shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto dt
                = std::chrono::duration<float, std::chrono::seconds::period>(
                    currentTime - lastTime);
            if (dt < frameMinDuration) {
                auto diff = frameMinDuration - dt;
                std::this_thread::sleep_for(diff);
                continue;
            }

            lastTime = currentTime;
            float dtf = dt.count();
            std::cout << "FPS: " << 1 / dtf << "\n";

            context.pollEvents();
            renderer.setViewMatrix(game.getCamera().getViewMatrix());
            game.update(dtf);
            renderer.setScene(
                &game.getScene()); // TODO set scene only when changes occured
            renderer.drawFrame();
        }
        renderer.deviceWaitIdle();

    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
