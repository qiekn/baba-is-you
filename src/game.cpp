#include "game.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <imgui.h>
#include <raylib.h>
#include <rlimgui.h>

const Game::Theme Game::kThemes[6] = {
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

namespace {
Rectangle CenteredRect(float width, float height, float offset_y = 0.0f) {
  return {
      (GetScreenWidth() - width) * 0.5f,
      (GetScreenHeight() - height) * 0.5f + offset_y,
      width,
      height,
  };
}

float GetDpiScale() {
  Vector2 dpi = GetWindowScaleDPI();
  return std::max(1.0f, std::max(dpi.x, dpi.y));
}
}  // namespace

void Game::Run() {
  Init();

  while (!WindowShouldClose()) {
    Tick();
  }

  Shutdown();
}

void Game::Init() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
  InitWindow(kScreenWidth, kScreenHeight, "baba");
  SetTargetFPS(kTargetFps);

  const float dpi_scale = GetDpiScale();

  rlImGuiBeginInitImGui();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  const float font_size = kImGuiBaseFontSize * dpi_scale;
  const std::filesystem::path regular_font = std::filesystem::path{"assets/fonts/opensans/OpenSans-Regular.ttf"};
  const std::filesystem::path bold_font = std::filesystem::path{"assets/fonts/opensans/OpenSans-Bold.ttf"};

  io.Fonts->Clear();

  ImFontConfig regular_config;
  regular_config.SizePixels = font_size;
  regular_config.OversampleH = 4;
  regular_config.OversampleV = 4;
  regular_config.PixelSnapH = false;
  regular_config.RasterizerMultiply = 1.05f;

  ImFontConfig bold_config = regular_config;

  if (std::filesystem::exists(bold_font)) {
    io.Fonts->AddFontFromFileTTF(bold_font.string().c_str(), font_size, &bold_config);
  }

  if (std::filesystem::exists(regular_font)) {
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
        regular_font.string().c_str(), font_size, &regular_config);
  }

  if (io.FontDefault == nullptr) {
    io.FontDefault = io.Fonts->AddFontDefault(&regular_config);
  }

  SetImGuiStyle(dpi_scale);

  rlImGuiEndInitImGui();
  ApplyTheme(selected_theme_);
}

void Game::Tick() {
  Update();
  Render();
}

void Game::Update() {}

void Game::Render() {
  BeginDrawing();
  ClearBackground(ToRaylibColor(background_color_));

  DrawGridBackground();

  rlImGuiBegin();
  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
  DrawEditorPanel();
  rlImGuiEnd();

  EndDrawing();
}

void Game::Shutdown() {
  rlImGuiShutdown();
  CloseWindow();
}

void Game::DrawGridBackground() const {
  const float board_width = kBoardCols * kCellPitch;
  const float board_height = kBoardRows * kCellPitch;
  const Rectangle board = CenteredRect(board_width, board_height, 12.0f);
  const Rectangle board_background = {
      board.x - kBoardPadding,
      board.y - kBoardPadding,
      board.width + kBoardPadding * 2.0f,
      board.height + kBoardPadding * 2.0f,
  };

  DrawRectangleRounded(board_background, 0.04f, 10, ToRaylibColor(board_background_color_));

  const Color border = ToRaylibColor(board_grid_border_color_);
  const float inset = (kCellPitch - kCellInnerSize) * 0.5f;
  const float cell_roundness = 0.18f;

  for (int row = 0; row < kBoardRows; ++row) {
    for (int col = 0; col < kBoardCols; ++col) {
      Rectangle cell = {
          board.x + col * kCellPitch + inset,
          board.y + row * kCellPitch + inset,
          kCellInnerSize,
          kCellInnerSize,
      };
      DrawRectangleRoundedLinesEx(cell, cell_roundness, 8, 2.5f, border);
    }
  }
}

void Game::DrawEditorPanel() {
  ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_Once);

  if (!ImGui::Begin("Baba Themes")) {
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
  ImGui::Separator();
  ImGui::Text("Board: %d x %d", kBoardCols, kBoardRows);
  ImGui::Text("Cell Pitch: %.0f", kCellPitch);
  ImGui::Text("Cell Inner Size: %.0f", kCellInnerSize);
  ImGui::Text("ImGui Base Font: %.0fpx", kImGuiBaseFontSize);

  ImGui::End();
}

void Game::ApplyTheme(int index) {
  selected_theme_ = std::clamp(index, 0, (int)(sizeof(kThemes) / sizeof(kThemes[0])) - 1);
  background_color_ = kThemes[selected_theme_].background_color;
  board_background_color_ = kThemes[selected_theme_].board_background_color;
  board_grid_border_color_ = kThemes[selected_theme_].board_grid_border_color;
}

void Game::SetImGuiStyle(float dpi_scale) {
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding = 8.0f;
  style.GrabRounding = 8.0f;
  style.TabRounding = 8.0f;
  style.ScrollbarRounding = 8.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
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

Color Game::ToRaylibColor(const ColorValue& color) {
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
