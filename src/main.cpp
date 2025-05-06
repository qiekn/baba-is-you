#include <math.h>

#include <cstddef>

#include "constants.h"
#include "raylib.h"

enum GameScreen { LOGO, TITLE, GAMEPLAY, ENDING };

void Update() {}

void DrawGrid(int rows = 18, int cols = 22, int cell_size = 24,
              int scale = kScale) {
  int sz = cell_size * scale;
  Color color = Color{47, 34, 86, 255};
  // horizontal lines
  for (int i = 0; i <= rows; i++) DrawLine(0, i * sz, cols * sz, i * sz, color);
  // vertical lines
  for (int i = 0; i <= cols; i++) DrawLine(i * sz, 0, i * sz, rows * sz, color);
}

void DrawScreenArea(int border = 5) {
  DrawText("SCREEN AREA", kScreenWidth - 160, 10, 20, RED);
  DrawRectangle(0, 0, kScreenWidth, border, RED);
  DrawRectangle(0, border, border, kScreenHeight - 10, RED);
  DrawRectangle(kScreenWidth - border, border, border, kScreenHeight - 10, RED);
  DrawRectangle(0, kScreenHeight - border, kScreenWidth, border, RED);
}

void DrawLogo() {
  // TODO: Draw LOGO screen here!
  ClearBackground(LIGHTGRAY);
  DrawText("LOGO SCREEN", 20, 20, 40, LIGHTGRAY);
  DrawText("WAIT for 2 SECONDS...", 290, 220, 20, GRAY);
}

void DrawTitle() {
  // TODO: Draw TITLE screen here!
  ClearBackground(GREEN);
  DrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
  DrawText("PRESS ENTER or TAP to JUMP to GAMEPLAY SCREEN", 120, 220, 20,
           DARKGREEN);
}

void DrawGame() {
  auto background_outer = Color{38, 30, 67, 255};
  auto background_inner = Color{26, 19, 35, 255};
  ClearBackground(background_outer);
  auto height = kCellSize * kScale * kRows;
  auto width = kCellSize * kScale * kCols;
  DrawRectangle(0, 0, width, height, background_inner);
  DrawGrid();
}

void DrawEnding() {
  // TODO: Draw ENDING screen here!
  ClearBackground(BLUE);
  DrawText("ENDING SCREEN", 20, 20, 40, DARKBLUE);
  DrawText("PRESS ENTER or TAP to RETURN to TITLE SCREEN", 120, 220, 20,
           DARKBLUE);
}

void Draw(int currentScreen) {
  switch (currentScreen) {
    case LOGO: {
      DrawLogo();
    } break;
    case TITLE: {
      DrawTitle();
    } break;
    case GAMEPLAY: {
      DrawGame();
    } break;
    case ENDING: {
      DrawEnding();
    } break;
    default:
      break;
  }
};

int main() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);  // vsync and high dpi
  SetConfigFlags(FLAG_MSAA_4X_HINT);                      // anti-aliasing
  SetTraceLogLevel(LOG_WARNING);

  SetTargetFPS(60);

  GameScreen currentScreen = GAMEPLAY;
  int framesCounter = 0;

  Camera2D camera = {0};
  camera.target = (Vector2){24 * 2 * 22 * 0.5, 24 * 2 * 18 * 0.5};
  camera.offset = (Vector2){kScreenWidth / 2.0f, kScreenHeight / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // simple scene manager
  switch (currentScreen) {
    case LOGO: {
      framesCounter++;  // Count frames
      // Wait for 2 seconds (120 frames) before jumping to TITLE screen
      if (framesCounter > 120) {
        currentScreen = TITLE;
      }
    } break;
    case TITLE: {
      if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
        currentScreen = GAMEPLAY;
      }
    } break;
    case GAMEPLAY: {
    } break;
    case ENDING: {
      // Press enter to return to TITLE screen
      if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
        currentScreen = TITLE;
      }
    } break;
    default:
      break;
  }

  InitWindow(kScreenWidth, kScreenHeight, "baba-is-you");

  while (!WindowShouldClose()) {
    Update();

    // Render
    BeginDrawing();

    // Game
    BeginMode2D(camera);
    Draw(currentScreen);
    EndMode2D();

    // UI
    DrawScreenArea();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
