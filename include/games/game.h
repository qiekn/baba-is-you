#pragma once

#include <raylib.h>
#include "managers/scene-manager.h"
class Game {
public:
  Game() {}
  virtual ~Game() {}

  void Run();

private:
  void Update();
  void Draw();

  bool is_running_ = false;
  SceneManager scene_manager_;
};
