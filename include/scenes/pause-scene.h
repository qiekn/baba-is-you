#pragma once

#include "scene.h"

class PauseScene : public Scene {
public:
  PauseScene() = default;
  virtual ~PauseScene() = default;

  void Update() override;
  void Draw() override;

private:
  /* data */
};
