#pragma once

#include "scene.h"

class TitleScene : public Scene {
public:
  TitleScene() = default;
  virtual ~TitleScene() = default;

  void Update() override;
  void Draw() override;

private:
  /* data */
};
