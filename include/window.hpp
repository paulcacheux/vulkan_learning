#ifndef WINDOW_TUTO_HPP
#define WINDOW_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <string>

#include "vulkan/renderer.hpp"

class Game;

namespace app {
class WindowContext {
  public:
    WindowContext();
    ~WindowContext();

    void pollEvents() const;
};

struct GameRendererCoupler {
    Game& game;
    vulkan::Renderer& renderer;
};

class Window {
  public:
    Window(std::size_t width, std::size_t height, std::string title);
    ~Window();

    GLFWwindow* inner() const;
    bool shouldClose() const;
    std::pair<int, int> getSize() const;
    std::pair<int, int> getFrameBufferSize() const;
    void switchToRawMouseMode() const;
    void linkToCoupler(GameRendererCoupler* coupler) const;
    void waitUntilUnminimized() const;

  private:
    GLFWwindow* _window;
    std::string _title;
};

void resize_callback(GLFWwindow* window, int, int);

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods);

void mouse_pos_callback(GLFWwindow* window, double xpos, double ypos);

} // namespace app

#endif
