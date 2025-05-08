#pragma once

#include <raylib.h>
#include "managers/scene-manager.h"

class Game {
public:
  Game();

  void Run();

private:
  void Init();
  void Update();
  void Draw();

  bool is_running_ = false;
  SceneManager scene_manager_;
};
