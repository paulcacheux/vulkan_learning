#ifndef GAME_TUTO_HPP
#define GAME_TUTO_HPP

#include <random>

#include "scene.hpp"

enum class InputState {
    Left,
    Right,
    Front,
    Back,
    Up,
    Down,
    MAX_ENUM,
};

constexpr std::size_t inputStateSize
    = static_cast<std::size_t>(InputState::MAX_ENUM);

class Game {
  public:
    Game();

    void update(float dt);
    bool getInputState(InputState is) const;
    void setInputState(InputState is, bool value);
    void setNewMouseInput(double xpos, double ypos);
    void randomChangeScene();

    const scene::Camera& getCamera() const;
    scene::Camera& getCamera();

  private:
    scene::Camera _camera;
    std::array<bool, inputStateSize> _states = {false};
    double _xpos = 0, _ypos = 0;
    glm::vec2 _currentMouseVec{};

    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_real_distribution<float> _distribution;
};

template <class R> glm::vec3 randomColor(R rand) {
    glm::vec3 color;
    for (std::size_t i = 0; i < 3; ++i) {
        color[i] = rand();
    }
    return color;
}

/*
template <class R>
void addCube(scene::Scene& scene, glm::vec3 origin, float width, R rand) {
    auto indexOffset = scene.vertices.size();

    scene.vertices.emplace_back(origin, randomColor(rand), glm::vec2{1.0, 0.0});
    scene.vertices.emplace_back(origin + glm::vec3(width, 0, 0),
                                randomColor(rand), glm::vec2{0.0, 0.0});
    scene.vertices.emplace_back(origin + glm::vec3(width, width, 0),
                                randomColor(rand), glm::vec2{0.0, 1.0});
    scene.vertices.emplace_back(origin + glm::vec3(0, width, 0),
                                randomColor(rand), glm::vec2{1.0, 1.0});
    scene.vertices.emplace_back(origin + glm::vec3(0, 0, width),
                                randomColor(rand), glm::vec2{1.0, 0.0});
    scene.vertices.emplace_back(origin + glm::vec3(width, 0, width),
                                randomColor(rand), glm::vec2{0.0, 0.0});
    scene.vertices.emplace_back(origin + glm::vec3(width, width, width),
                                randomColor(rand), glm::vec2{0.0, 1.0});
    scene.vertices.emplace_back(origin + glm::vec3(0, width, width),
                                randomColor(rand), glm::vec2{1.0, 1.0});

    scene.addTriangle({2, 1, 0}, indexOffset);
    scene.addTriangle({0, 3, 2}, indexOffset);
    scene.addTriangle({4, 5, 6}, indexOffset);
    scene.addTriangle({6, 7, 4}, indexOffset);
    scene.addTriangle({4, 3, 0}, indexOffset);
    scene.addTriangle({3, 4, 7}, indexOffset);
    scene.addTriangle({1, 2, 5}, indexOffset);
    scene.addTriangle({6, 5, 2}, indexOffset);
    scene.addTriangle({0, 1, 4}, indexOffset);
    scene.addTriangle({5, 4, 1}, indexOffset);
    scene.addTriangle({7, 2, 3}, indexOffset);
    scene.addTriangle({2, 7, 6}, indexOffset);
}*/

#endif
