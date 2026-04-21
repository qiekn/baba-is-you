#include "game.h"

#include <raylib.h>
#include <rlimgui.h>

namespace {
Rectangle CenteredRect(float width, float height, float offset_y = 0.0f) {
  return {
      (GetScreenWidth() - width) * 0.5f,
      (GetScreenHeight() - height) * 0.5f + offset_y,
      width,
      height,
  };
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

  rlImGuiBegin();
  ui_.Draw();
  rlImGuiEnd();

  EndDrawing();
}

void Game::Shutdown() {
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
