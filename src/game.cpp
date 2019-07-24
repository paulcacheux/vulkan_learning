#include "game.hpp"

#include <iostream>

Game::Game(scene::Scene s, scene::Camera c) : _scene(s), _camera(c) {
}

void Game::update(float dt) {
    float speed = 2.0f;
    auto offset = dt * speed;
#define UPDATE(dir)                                                            \
    if (getInputState(InputState::dir)) {                                      \
        _camera.move##dir(offset);                                             \
    }
    UPDATE(Left)
    UPDATE(Right)
    UPDATE(Up)
    UPDATE(Down)
    UPDATE(Front)
    UPDATE(Back)
    _camera.updateViewTarget(0.004f * _currentMouseVec);
    _currentMouseVec = glm::vec2(.0f);
}

bool Game::getInputState(InputState is) const {
    return _states[static_cast<std::size_t>(is)];
}

void Game::setInputState(InputState is, bool value) {
    _states[static_cast<std::size_t>(is)] = value;
}

void Game::setNewMouseInput(double xpos, double ypos) {
    glm::vec2 offset(xpos - _xpos, ypos - _ypos);
    _currentMouseVec += offset;
    _xpos = xpos;
    _ypos = ypos;
}

const scene::Scene& Game::getScene() const {
    return _scene;
}

const scene::Camera& Game::getCamera() const {
    return _camera;
}
