#pragma once

enum class SceneId {
  kLogo,
  kMenu,
  kGame,
  kTitle,
  kPause,
};

class Scene {
public:
  virtual void Update() = 0;
  virtual void Draw() = 0;

  Scene() = default;
  virtual ~Scene() = default;
};
