#include "managers/scene-manager.h"
#include <raylib.h>
#include <memory>
#include "scenes/game-scene.h"
#include "scenes/logo-scene.h"
#include "scenes/menu-scene.h"
#include "scenes/pause-scene.h"
#include "scenes/scene.h"
#include "scenes/title-scene.h"

SceneManager::SceneManager() : current_scene_id_(SceneId::kGame) {
  scenes_map_.emplace(SceneId::kLogo, std::make_unique<LogoScene>());
  scenes_map_.emplace(SceneId::kMenu, std::make_unique<MenuScene>());
  scenes_map_.emplace(SceneId::kGame, std::make_unique<GameScene>());
  scenes_map_.emplace(SceneId::kPause, std::make_unique<PauseScene>());
  scenes_map_.emplace(SceneId::kTitle, std::make_unique<TitleScene>());
}

SceneManager::~SceneManager() {};

void SceneManager::Update() {
  // switch scene
  switch (current_scene_id_) {
    case SceneId::kLogo:
      // wait for 2seconds (120 frames) before jumping to Title scene
      frame_counter++;
      if (frame_counter > 120) {
        current_scene_id_ = SceneId::kTitle;
      }
      break;
    case SceneId::kGame:
      if (IsKeyPressed(KEY_P)) {
        current_scene_id_ = SceneId::kPause;
      }
      break;
    case SceneId::kTitle:
      // press any key -> switch to start game
      if (GetKeyPressed() != 0) {
        current_scene_id_ = SceneId::kGame;
      }
      break;
    case SceneId::kPause:
      if (IsKeyPressed(KEY_P) && IsKeyPressed(KEY_ESCAPE)) {
        current_scene_id_ = SceneId::kGame;
      }
      break;
    default:
      break;
  }
  // scene update
  auto it = scenes_map_.find(current_scene_id_);
  if (it != scenes_map_.end() && it->second) {
    it->second->Update();
  }
}

void SceneManager::Draw() {
  auto it = scenes_map_.find(current_scene_id_);
  if (it != scenes_map_.end() && it->second) {
    it->second->Draw();
  }
}
