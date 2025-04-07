#include "raylib.h"

#include "resource_dir.h" // utility header for SearchAndSetResourceDir
#include <math.h>

int main() {
  // Tell the window to use vsync and work on high DPI displays
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

  const int screenWeight = 640;
  const int screenHeight = 480;

  SetConfigFlags(FLAG_MSAA_4X_HINT);

  // Create the window and OpenGL context
  InitWindow(screenWeight, screenHeight, "baba-is-you");

  // Utility function from resource_dir.h to find the resources folder and set
  // it as the current working directory so we can load from it
  SearchAndSetResourceDir("resources");

  // Load a texture from the resources directory
  Texture wabbit = LoadTexture("wabbit_alpha.png");

  // Init camera
  Camera3D camera = {0};
  camera.position = (Vector3){40.0f, 20.0f, 0.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 70.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  SetTargetFPS(60);
  int angle = 0;

  // game loop
  // run the loop untill the user presses ESCAPE or
  // presses the Close button on the window
  while (!WindowShouldClose()) {

    double time = GetTime();
    double cameraTime = time * 0.45;
    camera.position.x = (float)cos(cameraTime) * 40.0f;
    camera.position.z = (float)sin(cameraTime) * 40.0f;

    BeginDrawing();

    // Setup the back buffer for drawing (clear color and depth buffers)
    ClearBackground(WHITE);

    BeginMode3D(camera);
    DrawCube(Vector3{0, 0, 0}, 10, 10, 10, VIOLET);
    DrawCubeWires(Vector3{0, 0, 0}, 10, 10, 10, BLACK);
    EndMode3D();

    EndDrawing();
  }

  // destroy the window and cleanup the OpenGL context
  CloseWindow();
  return 0;
}
