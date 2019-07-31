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

void Window::linkToCoupler(GameRendererCoupler* coupler) const {
    glfwSetWindowUserPointer(_window, coupler);
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
    auto coupler = reinterpret_cast<GameRendererCoupler*>(
        glfwGetWindowUserPointer(window));
    coupler->renderer.setMustRecreateSwapchain();
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action,
                  int /*mods*/) {
    bool pressed;
    if (action == GLFW_PRESS) {
        pressed = true;
    } else if (action == GLFW_RELEASE) {
        pressed = false;
    } else {
        return;
    }

    auto coupler = reinterpret_cast<GameRendererCoupler*>(
        glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && pressed) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_LEFT_SHIFT) {
        coupler->game.setInputState(InputState::Down, pressed);
    } else if (key == GLFW_KEY_SPACE) {
        coupler->game.setInputState(InputState::Up, pressed);
    } else if (key == GLFW_KEY_W) {
        coupler->game.setInputState(InputState::Front, pressed);
    } else if (key == GLFW_KEY_A) {
        coupler->game.setInputState(InputState::Left, pressed);
    } else if (key == GLFW_KEY_S) {
        coupler->game.setInputState(InputState::Back, pressed);
    } else if (key == GLFW_KEY_D) {
        coupler->game.setInputState(InputState::Right, pressed);
    } else if (key == GLFW_KEY_P && pressed) {
        if (coupler->paused) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
            glfwSetCursorPosCallback(window, mouse_pos_callback);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            glfwSetCursorPosCallback(window, nullptr);
        }
        coupler->paused ^= true;
    }
}

void mouse_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    auto coupler = reinterpret_cast<GameRendererCoupler*>(
        glfwGetWindowUserPointer(window));
    coupler->game.setNewMouseInput(xpos, ypos);
}

} // namespace app
