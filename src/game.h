#pragma once

#include "ui.h"
#include <raylib.h>

struct Game {
  void Run();

private:
  void Init();
  void Tick();
  void Update();
  void Render();
  void Shutdown();

  void DrawGridBackground() const;

  static constexpr int kScreenWidth = 1280;
  static constexpr int kScreenHeight = 720;
  static constexpr int kTargetFps = 60;

  static constexpr int kBoardCols = 24;
  static constexpr int kBoardRows = 18;
  static constexpr float kCellPitch = 50.0f;
  static constexpr float kCellInnerSize = 44.0f;
  static constexpr float kBoardPadding = 28.0f;

  Ui ui_;
};
