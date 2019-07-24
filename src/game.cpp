#include "game.hpp"

#include <iostream>

Game::Game(scene::Scene s, scene::Camera c) : _scene(s), _camera(c) {
}

void Game::update(float dt) {
    float speed = 1.0f;
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
}

bool Game::getInputState(InputState is) const {
    return _states[static_cast<std::size_t>(is)];
}

void Game::setInputState(InputState is, bool value) {
    _states[static_cast<std::size_t>(is)] = value;
}

const scene::Scene& Game::getScene() const {
    return _scene;
}

const scene::Camera& Game::getCamera() const {
    return _camera;
}
