#pragma once

#include <raylib.h>

struct Ui {
  void Init();
  void Draw();
  void Shutdown();

  void ToggleVisible() { show_ui_ = !show_ui_; }
  bool IsVisible() const { return show_ui_; }

  Color BackgroundColor() const { return ToRaylibColor(background_color_); }
  Color BoardBackgroundColor() const { return ToRaylibColor(board_background_color_); }
  Color BoardGridBorderColor() const { return ToRaylibColor(board_grid_border_color_); }

private:
  struct ColorValue {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    float* data() { return &r; }
    const float* data() const { return &r; }
  };

  struct Theme {
    const char* name;
    ColorValue background;
    ColorValue board_background;
    ColorValue board_grid_border;
  };

  void DrawMainMenuBar();
  void DrawViewportPanel();
  void DrawScenePanel();
  void DrawInspectorPanel();
  void DrawRulesPanel();
  void DrawThemesPanel();

  void ApplyTheme(int index);
  void LoadFonts(float dpi_scale);
  void SetupStyle(float dpi_scale);

  static Color ToRaylibColor(const ColorValue& color);
  static float GetDpiScale();

  static constexpr float kImGuiBaseFontSize = 18.0f;
  static const Theme kThemes[6];

  bool show_ui_ = true;
  bool show_viewport_ = true;
  bool show_scene_ = true;
  bool show_inspector_ = true;
  bool show_rules_ = true;
  bool show_themes_ = true;
  bool show_demo_ = false;

  int selected_theme_ = 0;
  ColorValue background_color_{};
  ColorValue board_background_color_{};
  ColorValue board_grid_border_color_{};
};
