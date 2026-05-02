#include "imgui_layer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include "raylib.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "board.h"

namespace {

// When the dockspace node hosting the Viewport panel only contains the
// viewport itself, hide the tab bar for a clean editor look. As soon as the
// user docks another panel onto the same node, the tab bar re-appears so they
// can switch between siblings. Mirrors ck-engine's HideDockNodeTabBar.
void HideDockNodeTabBarIfSolo() {
  if (!ImGui::IsWindowDocked()) return;
  ImGuiDockNode* node = ImGui::GetWindowDockNode();
  if (!node) return;
  if (node->Windows.Size <= 1) {
    node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
  } else {
    node->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
  }
}

}  // namespace

const ImGuiLayer::Theme ImGuiLayer::kThemes[7] = {
    {
        "Default",
        {0.051f, 0.063f, 0.106f, 1.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
        {200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 100.0f / 255.0f},
    },
    {
        "Classic Sand",
        {0.91f, 0.90f, 0.86f, 1.0f},
        {0.82f, 0.79f, 0.72f, 1.0f},
        {0.18f, 0.17f, 0.15f, 40.0f / 255.0f},
    },
    {
        "Forest Moss",
        {0.77f, 0.82f, 0.73f, 1.0f},
        {0.54f, 0.63f, 0.45f, 1.0f},
        {0.18f, 0.26f, 0.14f, 40.0f / 255.0f},
    },
    {
        "Lake Blue",
        {0.77f, 0.86f, 0.93f, 1.0f},
        {0.55f, 0.70f, 0.81f, 1.0f},
        {0.14f, 0.26f, 0.35f, 40.0f / 255.0f},
    },
    {
        "Ash World",
        {0.72f, 0.70f, 0.72f, 1.0f},
        {0.53f, 0.52f, 0.56f, 1.0f},
        {0.16f, 0.15f, 0.18f, 40.0f / 255.0f},
    },
    {
        "Lava Glow",
        {0.94f, 0.79f, 0.58f, 1.0f},
        {0.82f, 0.45f, 0.25f, 1.0f},
        {0.29f, 0.08f, 0.05f, 40.0f / 255.0f},
    },
    {
        "Night Void",
        {0.15f, 0.16f, 0.22f, 1.0f},
        {0.24f, 0.26f, 0.34f, 1.0f},
        {0.86f, 0.88f, 0.94f, 40.0f / 255.0f},
    },
};

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

void ImGuiLayer::BindGamePanelToggles(bool* scene, bool* rules, bool* editor,
                                      bool* world, bool* settings) {
  show_scene_panel_ = scene;
  show_rules_panel_ = rules;
  show_editor_panel_ = editor;
  show_world_panel_ = world;
  show_settings_panel_ = settings;
}

void ImGuiLayer::OnAttach() {
  const float dpi_scale = GetDpiScale();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  LoadFonts(dpi_scale);
  SetupStyle(dpi_scale);

  GLFWwindow* window = glfwGetCurrentContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  ApplyTheme(selected_theme_);
}

void ImGuiLayer::OnDetach() {
  if (viewport_target_.id != 0) {
    UnloadRenderTexture(viewport_target_);
    viewport_target_ = RenderTexture2D{};
    viewport_target_w_ = viewport_target_h_ = 0;
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::OnUpdate(float /*dt*/) {
  if (IsKeyPressed(KEY_GRAVE)) {
    ToggleVisible();
  }
}

void ImGuiLayer::Begin() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::End() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow* backup = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup);
  }
}

void ImGuiLayer::OnImGuiRender() {
  // Visibility is gated by Game::Render before this runs, so no check here.

  // Plain dockspace (no PassthruCentralNode): the central area is now owned
  // by the Viewport panel which renders the game framebuffer as ImGui::Image.
  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

  DrawMainMenuBar();

  if (show_viewport_) DrawViewportPanel();
  if (show_inspector_) DrawInspectorPanel();
  if (show_themes_) DrawThemesPanel();

  if (show_demo_) {
    ImGui::ShowDemoWindow(&show_demo_);
  }
}

void ImGuiLayer::DrawMainMenuBar() {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("File")) {
    ImGui::MenuItem("Open Level...", nullptr, nullptr, false);
    ImGui::MenuItem("Save Level", nullptr, nullptr, false);
    ImGui::Separator();
    ImGui::MenuItem("Quit", "Alt+F4", nullptr, false);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (show_scene_panel_) ImGui::MenuItem("Scene", nullptr, show_scene_panel_);
    if (show_rules_panel_) ImGui::MenuItem("Rules", nullptr, show_rules_panel_);
    if (show_editor_panel_) ImGui::MenuItem("Editor", nullptr, show_editor_panel_);
    if (show_world_panel_) ImGui::MenuItem("World", nullptr, show_world_panel_);
    if (show_settings_panel_) ImGui::MenuItem("Settings", nullptr, show_settings_panel_);
    if (show_scene_panel_ || show_rules_panel_ || show_editor_panel_ ||
        show_world_panel_ || show_settings_panel_) {
      ImGui::Separator();
    }
    ImGui::MenuItem("Viewport", nullptr, &show_viewport_);
    ImGui::MenuItem("Inspector", nullptr, &show_inspector_);
    ImGui::MenuItem("Themes", nullptr, &show_themes_);
    ImGui::Separator();
    ImGuiIO& io = ImGui::GetIO();
    bool viewports = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
    if (ImGui::MenuItem("Allow Detach", nullptr, &viewports)) {
      if (viewports) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
      } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
      }
    }
    ImGui::Separator();
    ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    ImGui::TextDisabled("Toggle UI: `");
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

RenderTexture2D& ImGuiLayer::ViewportTarget() {
  // Choose a positive size: when the panel is closed/minimized or the UI is
  // hidden, fall back to the OS window size so the game still has somewhere
  // to draw to.
  int want_w = static_cast<int>(viewport_size_.x);
  int want_h = static_cast<int>(viewport_size_.y);
  if (want_w <= 0 || want_h <= 0) {
    want_w = std::max(1, GetScreenWidth());
    want_h = std::max(1, GetScreenHeight());
  }
  if (want_w != viewport_target_w_ || want_h != viewport_target_h_ || viewport_target_.id == 0) {
    if (viewport_target_.id != 0) UnloadRenderTexture(viewport_target_);
    viewport_target_ = LoadRenderTexture(want_w, want_h);
    SetTextureFilter(viewport_target_.texture, TEXTURE_FILTER_BILINEAR);
    viewport_target_w_ = want_w;
    viewport_target_h_ = want_h;
  }
  return viewport_target_;
}

void ImGuiLayer::DrawViewportPanel() {
  // Zero padding so the FBO image touches the panel borders and tab bar
  // toggling doesn't shift the rendered area.
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
  if (!ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    ImGui::PopStyleVar();
    return;
  }

  HideDockNodeTabBarIfSolo();

  const ImVec2 size = ImGui::GetContentRegionAvail();
  const ImVec2 pos = ImGui::GetCursorScreenPos();
  viewport_size_ = {size.x, size.y};
  viewport_top_left_ = {pos.x, pos.y};
  viewport_focused_ = ImGui::IsWindowFocused();
  viewport_hovered_ = ImGui::IsWindowHovered();
  // Publish to the board namespace so non-ImGui code (game_layer mouse
  // handling, BeginTextureMode-space drawing) can resolve viewport-local
  // coordinates without depending on ImGuiLayer directly.
  board::SetViewportOrigin(viewport_top_left_);
  board::kViewportHovered = viewport_hovered_;

  // raylib RenderTexture is upside-down relative to ImGui, so we flip V.
  if (viewport_target_.id != 0 && size.x > 0.0f && size.y > 0.0f) {
    ImGui::Image(static_cast<ImTextureID>(viewport_target_.texture.id),
                 size, ImVec2{0.0f, 1.0f}, ImVec2{1.0f, 0.0f});
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

void ImGuiLayer::DrawInspectorPanel() {
  if (!ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }
  ImGui::TextDisabled("Nothing selected.");
  ImGui::End();
}

void ImGuiLayer::DrawThemesPanel() {
  if (!ImGui::Begin("Themes", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  ImGui::Text("Baba-style board palette presets.");
  ImGui::Separator();

  if (ImGui::BeginCombo("Theme", kThemes[selected_theme_].name)) {
    for (int i = 0; i < (int)(sizeof(kThemes) / sizeof(kThemes[0])); ++i) {
      const bool is_selected = selected_theme_ == i;
      if (ImGui::Selectable(kThemes[i].name, is_selected)) {
        ApplyTheme(i);
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  if (ImGui::Button("Reset Theme")) {
    ApplyTheme(selected_theme_);
  }

  ImGui::Separator();
  ImGui::ColorEdit4("Background Color", background_color_.data());
  ImGui::ColorEdit4("Board Background", board_background_color_.data());
  ImGui::ColorEdit4("Grid Border", board_grid_border_color_.data());
  ImGui::Checkbox("Scale To Level", &board::kScaleToLevel);
  ImGui::Checkbox("Show Grid", &show_grid_);
  ImGui::SliderFloat("Grid Thickness", &board::kGridLineThickness, 0.5f, 4.0f, "%.2f px");
  ImGui::SliderFloat("Grid Opacity", &grid_opacity_, 0.0f, 1.0f, "%.2f");
  ImGui::TextDisabled("Tip: Ctrl + Left Click a slider to type a value.");
  ImGui::Separator();
  ImGui::Text("%.1f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

  ImGui::End();
}

void ImGuiLayer::ApplyTheme(int index) {
  selected_theme_ = std::clamp(index, 0, (int)(sizeof(kThemes) / sizeof(kThemes[0])) - 1);
  background_color_ = kThemes[selected_theme_].background;
  board_background_color_ = kThemes[selected_theme_].board_background;
  board_grid_border_color_ = kThemes[selected_theme_].board_grid_border;
}

void ImGuiLayer::LoadFonts(float dpi_scale) {
  ImGuiIO& io = ImGui::GetIO();

  const float font_size = kImGuiBaseFontSize * dpi_scale;
  const std::filesystem::path regular_font = std::filesystem::path{"assets/fonts/opensans/OpenSans-Regular.ttf"};
  const std::filesystem::path bold_font = std::filesystem::path{"assets/fonts/opensans/OpenSans-Bold.ttf"};

  io.Fonts->Clear();

  if (std::filesystem::exists(bold_font)) {
    io.Fonts->AddFontFromFileTTF(bold_font.string().c_str(), font_size);
  }

  if (std::filesystem::exists(regular_font)) {
    io.FontDefault = io.Fonts->AddFontFromFileTTF(regular_font.string().c_str(), font_size);
  }

  if (io.FontDefault == nullptr) {
    io.FontDefault = io.Fonts->AddFontDefault();
  }
}

void ImGuiLayer::SetupStyle(float dpi_scale) {
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding = 8.0f;
  style.GrabRounding = 8.0f;
  style.TabRounding = 8.0f;
  style.ScrollbarRounding = 8.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.WindowMenuButtonPosition = ImGuiDir_None;
  style.ScaleAllSizes(dpi_scale);

  auto& colors = style.Colors;
  colors[ImGuiCol_WindowBg] = ImVec4{0.10f, 0.105f, 0.11f, 1.0f};
  colors[ImGuiCol_Header] = ImVec4{0.20f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_HeaderHovered] = ImVec4{0.30f, 0.305f, 0.31f, 1.0f};
  colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_Button] = ImVec4{0.20f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_ButtonHovered] = ImVec4{0.30f, 0.305f, 0.31f, 1.0f};
  colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_FrameBg] = ImVec4{0.20f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_FrameBgHovered] = ImVec4{0.30f, 0.305f, 0.31f, 1.0f};
  colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
  colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
  colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.20f, 0.205f, 0.21f, 1.0f};
  colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
  colors[ImGuiCol_DockingEmptyBg] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};
}

Color ImGuiLayer::ToRaylibColor(const ColorValue& color) {
  auto to_byte = [](float value) {
    return (unsigned char)std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f);
  };

  return {
      to_byte(color.r),
      to_byte(color.g),
      to_byte(color.b),
      to_byte(color.a),
  };
}

float ImGuiLayer::GetDpiScale() {
  Vector2 dpi = GetWindowScaleDPI();
  return std::max(1.0f, std::max(dpi.x, dpi.y));
}
