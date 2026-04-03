#pragma once

#include <raylib.h>

struct Game {
  void Run();

private:
  struct ColorValue {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    float* data() { return &r; }
    const float* data() const { return &r; }
  };

  struct Theme {
    const char* name;
    ColorValue background_color;
    ColorValue board_background_color;
    ColorValue board_grid_border_color;
  };

  void Init();
  void Tick();
  void Update();
  void Render();
  void Shutdown();

  void DrawGridBackground() const;
  void DrawEditorPanel();
  void ApplyTheme(int index);
  void SetImGuiStyle(float dpi_scale);
  static Color ToRaylibColor(const ColorValue& color);

  static constexpr int kScreenWidth = 1280;
  static constexpr int kScreenHeight = 720;
  static constexpr int kTargetFps = 60;

  static constexpr int kBoardCols = 24;
  static constexpr int kBoardRows = 18;
  static constexpr float kCellPitch = 50.0f;
  static constexpr float kCellInnerSize = 44.0f;
  static constexpr float kBoardPadding = 28.0f;
  static constexpr float kImGuiBaseFontSize = 18.0f;

  static const Theme kThemes[6];

  int selected_theme_{0};
  ColorValue background_color_{};
  ColorValue board_background_color_{};
  ColorValue board_grid_border_color_{};
};


