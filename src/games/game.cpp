#include "games/game.h"
#include <rlimgui.h>
#include "constants.h"

void Game::Run() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);  // vsync and high dpi
  SetConfigFlags(FLAG_MSAA_4X_HINT);                      // anti-aliasing
  SetTraceLogLevel(LOG_DEBUG);
  DrawFPS(0, 0);

  InitWindow(kScreenWidth, kScreenHeight, "game");
  SetTargetFPS(KFps);
  is_running_ = true;

  while (!WindowShouldClose() && is_running_) {
    Update();
    Draw();
  }
}

void Game::Update() { scene_manager_.Update(); }

void Game::Draw() {
  BeginDrawing();
  scene_manager_.Draw();
  EndDrawing();
}
