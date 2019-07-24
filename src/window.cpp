#include "window.hpp"

#include <iostream>

#include "game.hpp"

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

void Window::switchToRawMouseMode() const {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

void Window::linkToInstance(vulkan::Instance* instance) const {
    glfwSetWindowUserPointer(_window, instance);
    glfwSetKeyCallback(_window, key_callback);
    glfwSetFramebufferSizeCallback(_window, resize_callback);
    glfwSetCursorPosCallback(_window, mouse_pos_callback);
}

void Window::waitUntilUnminimized() const {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }
}

void resize_callback(GLFWwindow* window, int, int) {
    auto instance
        = reinterpret_cast<vulkan::Instance*>(glfwGetWindowUserPointer(window));
    instance->setMustRecreateSwapchain();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods) {
    bool pressed;
    if (action == GLFW_PRESS) {
        pressed = true;
    } else if (action == GLFW_RELEASE) {
        pressed = false;
    } else {
        return;
    }

    auto instance
        = reinterpret_cast<vulkan::Instance*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && pressed) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_LEFT_SHIFT) {
        instance->game.setInputState(InputState::Down, pressed);
    } else if (key == GLFW_KEY_SPACE) {
        instance->game.setInputState(InputState::Up, pressed);
    } else if (key == GLFW_KEY_W) {
        instance->game.setInputState(InputState::Front, pressed);
    } else if (key == GLFW_KEY_A) {
        instance->game.setInputState(InputState::Left, pressed);
    } else if (key == GLFW_KEY_S) {
        instance->game.setInputState(InputState::Back, pressed);
    } else if (key == GLFW_KEY_D) {
        instance->game.setInputState(InputState::Right, pressed);
    }
}

void mouse_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    auto instance
        = reinterpret_cast<vulkan::Instance*>(glfwGetWindowUserPointer(window));
    instance->game.setNewMouseInput(xpos, ypos);
}

} // namespace app
