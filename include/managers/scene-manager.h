#pragma once

#include <cstddef>
#include <unordered_map>
#include "scenes/scene.h"
class SceneManager {
public:
  SceneManager();
  virtual ~SceneManager();

  void Update();

  void Draw();

private:
  SceneId current_scene_id_;
  std::unordered_map<SceneId, std::unique_ptr<Scene>> scenes_map_;
  size_t frame_counter = 0;
};
