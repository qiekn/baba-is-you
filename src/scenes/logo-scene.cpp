module;

#include <raylib.h>

module baba;

void LogoScene::Update() {}

void LogoScene::Draw() {
  ClearBackground(LIGHTGRAY);
  DrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
  DrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);
}
