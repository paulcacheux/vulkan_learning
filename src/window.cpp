#include "window.hpp"

namespace app {

WindowContext::WindowContext() {
    glfwInit();
}

WindowContext::~WindowContext() {
    glfwTerminate();
}

void WindowContext::pollEvents() const {
    glfwPollEvents();
}

Window::Window(std::size_t width, std::size_t height, std::string title)
    : _title(title) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(width, height, _title.c_str(), nullptr, nullptr);

    glfwSetKeyCallback(_window, key_callback);
}

Window::~Window() {
    glfwDestroyWindow(_window);
}

GLFWwindow* Window::inner() const {
    return _window;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(_window);
}

std::pair<int, int> Window::getSize() const {
    int width, height;
    glfwGetWindowSize(_window, &width, &height);
    return std::make_pair(width, height);
}

std::pair<int, int> Window::getFrameBufferSize() const {
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    return std::make_pair(width, height);
}

void Window::linkResizeToVulkan(VulkanInstance* instance) {
    glfwSetWindowUserPointer(_window, instance);
    glfwSetFramebufferSizeCallback(_window, vulkan_resize_callback);
}

void vulkan_resize_callback(GLFWwindow* window, int, int) {
    auto app
        = reinterpret_cast<VulkanInstance*>(glfwGetWindowUserPointer(window));
    app->setMustRecreateSwapchain();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

} // namespace app
