#pragma once

#include "scene.h"

class MenuScene : public Scene {
public:
  MenuScene() = default;
  virtual ~MenuScene() = default;

  void Update() override;
  void Draw() override;

private:
  /* data */
};
