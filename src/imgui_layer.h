#pragma once

#include <raylib.h>

#include "layer.h"

class ImGuiLayer : public Layer {
 public:
  ImGuiLayer();

  void OnAttach() override;
  void OnDetach() override;
  void OnUpdate(float dt) override;
  void OnImGuiRender() override;

  // Begin/End bracket the per-frame ImGui pass around every layer's
  // OnImGuiRender. Keeping this explicit (rather than rolling it into
  // OnImGuiRender) lets the owner decide the exact ordering relative to
  // raylib draws and multi-viewport rendering.
  void Begin();
  void End();

  void ToggleVisible() { visible_ = !visible_; }
  bool IsVisible() const { return visible_; }

  Color BackgroundColor() const { return ToRaylibColor(background_color_); }
  Color BoardBackgroundColor() const { return ToRaylibColor(board_background_color_); }
  Color BoardGridBorderColor() const { return ToRaylibColor(board_grid_border_color_); }
  float GridOpacity() const { return grid_opacity_; }
  bool ShowGrid() const { return show_grid_; }
  void BindGamePanelToggles(bool* scene, bool* rules, bool* editor, bool* world, bool* settings);

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
  void DrawInspectorPanel();
  void DrawThemesPanel();

  void ApplyTheme(int index);
  void LoadFonts(float dpi_scale);
  void SetupStyle(float dpi_scale);

  static Color ToRaylibColor(const ColorValue& color);
  static float GetDpiScale();

  static constexpr float kImGuiBaseFontSize = 18.0f;
  static const Theme kThemes[7];

  bool visible_ = true;
  bool show_viewport_ = false;
  bool show_inspector_ = true;
  bool show_themes_ = true;
  bool show_demo_ = false;
  bool show_grid_ = true;
  float grid_opacity_ = 0.2f;
  bool* show_scene_panel_ = nullptr;
  bool* show_rules_panel_ = nullptr;
  bool* show_editor_panel_ = nullptr;
  bool* show_world_panel_ = nullptr;
  bool* show_settings_panel_ = nullptr;

  int selected_theme_ = 0;
  ColorValue background_color_{};
  ColorValue board_background_color_{};
  ColorValue board_grid_border_color_{};
};
