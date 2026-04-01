#include "game.h"
#include <raylib.h>

void Game::Run() {
  Init();

  while (!WindowShouldClose()) {
    Tick();
  }

  Shutdown();
}

void Game::Init() {
  InitWindow(kScreenWidth, kScreenHeight, "baba");
  SetTargetFPS(kTargetFps);
}

void Game::Tick() {
  Update();
  Render();
}

void Game::Update() {}

void Game::Render() {
  BeginDrawing();
  ClearBackground(RAYWHITE);
  DrawText("Hello from raylib", 40, 40, 32, DARKGRAY);
  DrawText("Press ESC to quit", 40, 88, 20, GRAY);
  EndDrawing();
}

void Game::Shutdown() { CloseWindow(); }