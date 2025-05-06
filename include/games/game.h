#pragma once

#include <raylib.h>
class Game {
public:
  Game() {}
  virtual ~Game() {}

  void Run();

private:
  bool is_running_ = false;
  Camera2D camera_ = {0};

  /* methods */
  void Update();
  void Draw();
};
