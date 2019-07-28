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

    scene::Scene getScene() const;
    const scene::Camera& getCamera() const;
    scene::Camera& getCamera();
    glm::mat4 getModelMatrix() const;

    bool sceneHasChanged = true;

  private:
    scene::Scene _scene;
    scene::Camera _camera;
    std::array<bool, inputStateSize> _states = {false};
    double _xpos = 0, _ypos = 0;
    glm::vec2 _currentMouseVec{};

    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_real_distribution<float> _distribution;
};

#endif
