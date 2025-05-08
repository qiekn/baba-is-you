#include "games/game.h"
#include <raylib.h>
#include <rlimgui.h>
#include <fstream>
#include <iostream>
#include "constants.h"
#include "games/object.h"
#include "magic-enum.h"
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
  // debug object enum
  // dump enum list to file
  if (IsKeyPressed(KEY_E)) {
    std::ofstream ofs("levels/enum_list.txt");
    if (!ofs.is_open()) {
      std::cerr << "can't open enum_list.txt!\n";
    } else {
      for (int i = static_cast<int>(ObjectType::DEFAULT);
           i <= static_cast<int>(ObjectType::ICON_END); i++) {
        auto type = static_cast<ObjectType>(i);
        ofs << magic_enum::enum_name(type) << " \t" << i << "\n";
      }
      std::cout << "update enum list success\n";
    }
    ofs.close();
  }
}

void Game::Draw() {
  BeginDrawing();
  scene_manager_.Draw();
  EndDrawing();
}
