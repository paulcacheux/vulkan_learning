#ifndef WINDOW_TUTO_HPP
#define WINDOW_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstddef>
#include <string>

#include "vulkan.hpp"

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

  private:
    GLFWwindow* _window;
    std::size_t _width, _height;
    std::string _title;
};

} // namespace app

#endif
