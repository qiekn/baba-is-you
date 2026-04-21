#include "game.h"

#include <raylib.h>
#include <rlgl.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

namespace {
constexpr const char* kWindowStateFile = "window.state";

Rectangle CenteredRect(float width, float height, float offset_y = 0.0f) {
  return {
      (GetScreenWidth() - width) * 0.5f,
      (GetScreenHeight() - height) * 0.5f + offset_y,
      width,
      height,
  };
}

struct WindowState {
  int x;
  int y;
  int w;
  int h;
};

WindowState LoadWindowState(int default_w, int default_h) {
  WindowState s{80, 80, default_w, default_h};
  if (FILE* f = std::fopen(kWindowStateFile, "r")) {
    int x, y, w, h;
    if (std::fscanf(f, "%d %d %d %d", &x, &y, &w, &h) == 4 && w > 0 && h > 0) {
      s = {x, y, w, h};
    }
    std::fclose(f);
  }
  return s;
}

void SaveWindowState() {
  Vector2 pos = GetWindowPosition();
  if (FILE* f = std::fopen(kWindowStateFile, "w")) {
    std::fprintf(f, "%d %d %d %d\n", (int)pos.x, (int)pos.y, GetScreenWidth(), GetScreenHeight());
    std::fclose(f);
  }
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
  const WindowState state = LoadWindowState(kScreenWidth, kScreenHeight);

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
  InitWindow(state.w, state.h, "baba");
  SetWindowPosition(state.x, state.y);
  SetTargetFPS(kTargetFps);

  ui_.Init();
}

void Game::Tick() {
  Update();
  Render();
}

void Game::Update() {
  if (IsKeyPressed(KEY_O)) {
    ui_.ToggleVisible();
  }
}

void Game::Render() {
  BeginDrawing();
  ClearBackground(ui_.BackgroundColor());

  DrawGridBackground();
  rlDrawRenderBatchActive();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ui_.Draw();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow* backup = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup);
  }

  EndDrawing();
}

void Game::Shutdown() {
  SaveWindowState();
  ui_.Shutdown();
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

  DrawRectangleRounded(board_background, 0.04f, 10, ui_.BoardBackgroundColor());

  const Color border = ui_.BoardGridBorderColor();
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
