#include "game.hpp"

#include <iostream>

Game::Game() : _rd(), _gen(_rd()), _distribution() {
    /* cube gen
    _scene.vertices.clear();
    _scene.indices.clear();

    auto gen = [this]() { return _distribution(_gen); };

    for (auto a = 0.0f; a < 0.3; a += 0.1f) {
        for (auto b = 0.0f; b < 0.3; b += 0.1f) {
            for (auto c = 0.0f; c < 0.3; c += 0.1f) {
                addCube(_scene, {-0.3 + a, -0.3 + b, -0.3 + c}, 0.1, gen);
            }
        }
    }*/

    // _scene = scene::Scene("../obj/cathedral/combined02.obj");
}

void Game::update(float dt) {
    float speed = 1.8f;
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
    _camera.updateViewTarget(0.05f * _currentMouseVec);
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

const scene::Camera& Game::getCamera() const {
    return _camera;
}

scene::Camera& Game::getCamera() {
    return _camera;
}
