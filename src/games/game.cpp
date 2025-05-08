#include "games/game.h"
#include <raylib.h>
#include <rlimgui.h>
#include "constants.h"
#include "maid.h"

Game::Game() {}

void Game::Run() {
  Init();
  TraceLog(LOG_DEBUG, "GAME ONNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN!");

  while (!WindowShouldClose() && is_running_) {
    Update();
    Draw();
  }
}

void Game::Init() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);  // vsync and high dpi
  SetConfigFlags(FLAG_MSAA_4X_HINT);                      // anti-aliasing
  SetTraceLogLevel(LOG_DEBUG);
  DrawFPS(0, 0);

  Maid::Instance().rule_manager_.Initialize();

  InitWindow(kScreenWidth, kScreenHeight, "game");
  SetTargetFPS(KFps);
  is_running_ = true;
}

void Game::Update() {
  scene_manager_.Update();
  if (IsKeyPressed(KEY_SPACE)) {
    Maid::Instance().rule_manager_.Update();
  }
}

void Game::Draw() {
  BeginDrawing();
  scene_manager_.Draw();
  EndDrawing();
}
