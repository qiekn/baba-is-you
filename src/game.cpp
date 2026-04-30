#include "game.h"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cstdio>
#include <memory>

#include "board.h"
#include "game_layer.h"

namespace {
constexpr const char* kWindowStateFile = "window.state";

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

void SetWindowIconFromPng(const char* path) {
  Image icon = LoadImage(path);
  if (icon.data != nullptr && icon.width > 0 && icon.height > 0) {
    SetWindowIcon(icon);
  }
  UnloadImage(icon);
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
  SetTraceLogLevel(LOG_WARNING);
  const WindowState state = LoadWindowState(kScreenWidth, kScreenHeight);

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
  InitWindow(state.w, state.h, "baba is you");
  SetWindowPosition(state.x, state.y);
  SetWindowIconFromPng("assets/icons/favicon.png");
  SetTargetFPS(kTargetFps);
  InitAudioDevice();

  // Default to borderless fullscreen — F11 toggles back to the windowed state
  // captured here (loaded from window.state).
  ToggleBorderless();

  // Initialize layers

  auto imgui_layer = std::make_unique<ImGuiLayer>();
  auto game_layer = std::make_unique<GameLayer>();

  imgui_layer_ = imgui_layer.get();
  game_layer_ = game_layer.get();
  imgui_layer_->BindGamePanelToggles(game_layer_->ShowScenePanelPtr(),
                                     game_layer_->ShowRulesPanelPtr(),
                                     game_layer_->ShowEditorPanelPtr(),
                                     game_layer_->ShowWorldPanelPtr(),
                                     game_layer_->ShowSettingsPanelPtr());

  layers_.push_layer(std::move(game_layer));
  layers_.push_overlay(std::move(imgui_layer));
}

void Game::Tick() {
  Update();
  Render();
}

void Game::Update() {
  const float dt = GetFrameTime();
  if (IsKeyPressed(KEY_F11)) {
    ToggleBorderless();
  }
  for (auto& layer : layers_) {
    layer->OnUpdate(dt);
  }
}

void Game::ToggleBorderless() {
  if (!borderless_) {
    windowed_pos_ = GetWindowPosition();
    windowed_size_ = {(float)GetScreenWidth(), (float)GetScreenHeight()};

    const int monitor = GetCurrentMonitor();
    const Vector2 mpos = GetMonitorPosition(monitor);
    const int mw = GetMonitorWidth(monitor);
    const int mh = GetMonitorHeight(monitor);

    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition((int)mpos.x, (int)mpos.y);
    // +1 px so Windows doesn't auto-promote this to exclusive fullscreen
    // (WS_POPUP + exact-monitor-size triggers fullscreen optimizations).
    SetWindowSize(mw, mh + 1);
    borderless_ = true;
  } else {
    ClearWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowSize((int)windowed_size_.x, (int)windowed_size_.y);
    SetWindowPosition((int)windowed_pos_.x, (int)windowed_pos_.y);
    borderless_ = false;
  }
}

void Game::Render() {
  BeginDrawing();
  const auto edge_bg = game_layer_ ? game_layer_->LevelEdge() : std::nullopt;
  ClearBackground(edge_bg.value_or(imgui_layer_->BackgroundColor()));

  DrawGridBackground();
  for (auto& layer : layers_) {
    layer->OnRender();
  }
  DrawGridOverlay();
  // Draw transition while we're still in raylib's normal 2D render state.
  // Rendering it after ImGui's OpenGL backend can leave driver state that
  // prevents the raylib triangles from showing reliably.
  if (game_layer_) game_layer_->DrawTransitionOverlay();
  rlDrawRenderBatchActive();

  imgui_layer_->Begin();
  if (imgui_layer_->IsVisible()) {
    for (auto& layer : layers_) {
      layer->OnImGuiRender();
    }
  }
  imgui_layer_->End();

  EndDrawing();
}

void Game::Shutdown() {
  if (borderless_) ToggleBorderless();
  SaveWindowState();
  layers_.clear();  // detach layers before the GL context goes away
  CloseAudioDevice();
  CloseWindow();
}

void Game::DrawGridBackground() const {
  const Rectangle board = board::BoardRect();
  const Rectangle board_background = board;

  DrawRectangleRounded(board_background, 0.04f, 10, imgui_layer_->BoardBackgroundColor());

  // Imported levels override the board interior with their palette color so
  // the themed background reads correctly even though our ImGui theme is
  // still active.
  if (game_layer_) {
    if (auto bg = game_layer_->LevelBackground()) {
      DrawRectangleRounded(board_background, 0.04f, 10, *bg);
    }
  }

}

void Game::DrawGridOverlay() const {
  if (!imgui_layer_->ShowGrid()) return;
  const Rectangle board = board::BoardRect();
  Color border = imgui_layer_->BoardGridBorderColor();
  const float opacity = std::clamp(imgui_layer_->GridOpacity(), 0.0f, 1.0f);
  border.a = static_cast<unsigned char>(border.a * opacity);
  const float t = board::kGridLineThickness;
  const float pitch = board::Pitch();
  const int cols = board::RenderCols();
  const int rows = board::RenderRows();

  // Draw all verticals first. Extend by half-thickness at top/bottom so the
  // outer corners stay complete even when horizontals avoid overlaps.
  for (int col = 0; col <= cols; ++col) {
    const float x = board.x + col * pitch - t * 0.5f;
    DrawRectangleRec({x, board.y - t * 0.5f, t, board.height + t}, border);
  }
  // Draw horizontals as segmented spans between vertical lines so grid
  // intersections are not painted twice (keeps opacity uniform).
  for (int row = 0; row <= rows; ++row) {
    const float y = board.y + row * pitch - t * 0.5f;
    for (int col = 0; col < cols; ++col) {
      const float x = board.x + col * pitch + t * 0.5f;
      const float w = std::max(0.0f, pitch - t);
      DrawRectangleRec({x, y, w, t}, border);
    }
  }
}
