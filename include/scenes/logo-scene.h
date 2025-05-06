#pragma once

#include "scenes/scene.h"

class LogoScene : public Scene {
public:
  LogoScene() = default;
  virtual ~LogoScene() = default;

  void Update() override;
  void Draw() override;

private:
};
