#pragma once

#include "scene.h"
#include "systems/render-system.h"

class GameScene : public Scene {
public:
  GameScene();
  virtual ~GameScene();

  void Update() override;
  void Draw() override;

private:
  RenderSystem& render_system_;
  Camera2D camera_ = {0};
};
