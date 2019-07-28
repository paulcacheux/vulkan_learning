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
        auto scene = game.getScene();
        vulkan::Renderer renderer(window);
        renderer.setScene(&scene);

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
            if (game.sceneHasChanged) {
                scene = game.getScene();
                renderer.setScene(&scene);
                game.sceneHasChanged = false;
                std::cout << "update\n";
            }
            renderer.drawFrame();
        }
        renderer.deviceWaitIdle();

    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
