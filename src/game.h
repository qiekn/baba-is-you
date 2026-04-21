#pragma once

#include <raylib.h>

#include "imgui_layer.h"
#include "layer_stack.h"

struct Game {
  void Run();

 private:
  void Init();
  void Tick();   // TimeStep or DeltaTime progress
  void Update(); // Update GameLogic
  void Render(); // Rendering
  void Shutdown();

  void DrawGridBackground() const;

  static constexpr int kScreenWidth = 1280;
  static constexpr int kScreenHeight = 720;
  static constexpr int kTargetFps = 160;

  static constexpr int kBoardCols = 24;
  static constexpr int kBoardRows = 18;
  static constexpr float kCellPitch = 50.0f;
  static constexpr float kCellInnerSize = 44.0f;
  static constexpr float kBoardPadding = 28.0f;

  LayerStack layers_;
  ImGuiLayer* imgui_layer_ = nullptr;  // non-owning; layers_ owns the unique_ptr
};
