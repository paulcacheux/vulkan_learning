#ifndef GAME_TUTO_HPP
#define GAME_TUTO_HPP

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
    Game(scene::Scene s, scene::Camera c);

    void update(float dt);
    bool getInputState(InputState is) const;
    void setInputState(InputState is, bool value);

    const scene::Scene& getScene() const;
    const scene::Camera& getCamera() const;

  private:
    scene::Scene _scene;
    scene::Camera _camera;
    std::array<bool, inputStateSize> _states = {false};
};

#endif
