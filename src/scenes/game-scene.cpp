#include "scenes/game-scene.h"
#include <raylib.h>
#include "constants.h"
#include "imgui.h"
#include "maid.h"
#include "rlimgui.h"
#include "systems/render-system.h"

GameScene::GameScene() : render_system_(Maid::Instance().render_system_) {
  // init game camera
  camera_.target = (Vector2){kCellSize * kCols * kScale * 0.5,
                             kCellSize * kRows * kScale * 0.5};
  camera_.offset = (Vector2){kScreenWidth / 2.0f, kScreenHeight / 2.0f};
  camera_.rotation = 0.0f;
  camera_.zoom = 1.0f;

  // init imgui
  rlImGuiSetup(true);
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.FontGlobalScale = 1.25f;
  ImGui::StyleColorsClassic();

  // load levels
  Maid::Instance().level_manager_.LoadJson();
  Maid::Instance().level_manager_.InitLevel();
}

GameScene::~GameScene() {}

void GameScene::Update() {}

void GameScene::Draw() {
  BeginMode2D(camera_);
  render_system_.DrawScene();
  EndMode2D();

  render_system_.DrawUI();

  /*
  rlImGuiBegin();
  ImGui::Begin("Camera Offset");
  ImGui::DragFloat2("Camera Offset", &camera_.offset.x, 1.0f, -1000.0f,
                    1000.0f);
  ImGui::End();
  rlImGuiEnd();
  */
}
