#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "game.hpp"
#include "scene.hpp"
#include "vulkan/context.hpp"
#include "vulkan/renderer.hpp"
#include "window.hpp"

const std::size_t WIDTH = 1000;
const std::size_t HEIGHT = 800;

class FpsWatcher {
  public:
    using duration = std::chrono::duration<float, std::chrono::seconds::period>;
    using time_point
        = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using clock = std::chrono::high_resolution_clock;

    FpsWatcher(int maxFps)
        : _minFrameDuration(1 / static_cast<float>(maxFps)),
          _lastTime(clock::now()), _lastPrintTime(clock::now()), _counter(0) {
    }

    std::optional<duration> checkFps() {
        auto currentTime = clock::now();
        if (duration(currentTime - _lastPrintTime) > duration(1.0f)) {
            std::cout << "FPS: " << _counter << "\n";
            _counter = 0;
            _lastPrintTime = currentTime;
        }

        auto dt = duration(currentTime - _lastTime);
        if (dt < _minFrameDuration) {
            auto diff = _minFrameDuration - dt;
            std::this_thread::sleep_for(diff);
            return {};
        }
        _lastTime = clock::now();
        ++_counter;
        return dt;
    }

  private:
    duration _minFrameDuration;
    time_point _lastTime, _lastPrintTime;
    int _counter;
};

int main() {
    try {
        app::WindowContext windowContext;
        app::Window window(WIDTH, HEIGHT, "Vulkan window");

        Game game;
        auto scene = game.getScene();

        vulkan::Context context(window.inner());
        vulkan::Renderer renderer(window, context);
        renderer.setScene(&scene);

        app::GameRendererCoupler coupler{game, renderer};
        window.linkToCoupler(&coupler);
        window.switchToRawMouseMode();

        FpsWatcher fps(300);

        while (!window.shouldClose()) {
            auto dt = fps.checkFps();
            if (!dt) {
                continue;
            }

            windowContext.pollEvents();
            renderer.setViewMatrix(game.getCamera().getViewMatrix());
            game.update(dt->count());
            if (game.sceneHasChanged) {
                scene = game.getScene();
                renderer.setScene(&scene);
                game.sceneHasChanged = false;
                std::cout << "update\n";
            }
            renderer.drawFrame();
        }
        renderer.deviceWaitIdle();

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
