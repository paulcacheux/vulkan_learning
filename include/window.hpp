#ifndef WINDOW_TUTO_HPP
#define WINDOW_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <string>

#include "vulkan/instance.hpp"

class Game;

namespace app {
class WindowContext {
  public:
    WindowContext();
    ~WindowContext();

    void pollEvents() const;
};

class Window {
  public:
    Window(std::size_t width, std::size_t height, std::string title);
    ~Window();

    GLFWwindow* inner() const;
    bool shouldClose() const;
    std::pair<int, int> getSize() const;
    std::pair<int, int> getFrameBufferSize() const;
    void linkToInstance(vulkan::Instance* instance);
    void setupResizeCallback();
    void waitUntilUnminimized() const;

  private:
    GLFWwindow* _window;
    std::string _title;
};

void vulkan_resize_callback(GLFWwindow* window, int, int);

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods);

} // namespace app

#endif
