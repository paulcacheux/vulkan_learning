#include "game.hpp"

#include <iostream>

Game::Game() : _rd(), _gen(_rd()), _distribution() {
    _scene.vertices = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    };

    _scene.indices.clear();
    _scene.addTriangle({2, 1, 0});
    _scene.addTriangle({0, 3, 2});
    _scene.addTriangle({4, 5, 6});
    _scene.addTriangle({6, 7, 4});
    _scene.addTriangle({4, 3, 0});
    _scene.addTriangle({3, 4, 7});
    _scene.addTriangle({1, 2, 5});
    _scene.addTriangle({6, 5, 2});
    _scene.addTriangle({0, 1, 4});
    _scene.addTriangle({5, 4, 1});
    _scene.addTriangle({7, 2, 3});
    _scene.addTriangle({2, 7, 6});
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

void Game::randomChangeScene() {
    for (auto& vertex : _scene.vertices) {
        // change color
        for (std::size_t i = 0; i < 3; ++i) {
            vertex.color[i] = _distribution(_gen);
        }
        // change cube
        /*for (std::size_t i = 0; i < 3; ++i) {
            vertex.pos[i] += (_distribution(_gen) - 0.5) / 10;
        }*/
    }
    sceneHasChanged = true;
}

scene::Scene Game::getScene() const {
    return _scene;
}

const scene::Camera& Game::getCamera() const {
    return _camera;
}

scene::Camera& Game::getCamera() {
    return _camera;
}

glm::mat4 Game::getModelMatrix() const {
    return glm::mat4();
}
