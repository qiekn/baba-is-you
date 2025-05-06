#include "scenes/title-scene.h"
#include <raylib.h>

void TitleScene::Update() {}

void TitleScene::Draw() {
  // TODO: Draw TITLE screen here!
  ClearBackground(GREEN);
  DrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
  DrawText("PRESS ENTER **ANYKEY** JUMP to GAMEPLAY SCREEN", 120, 220, 20,
           DARKGREEN);
}
