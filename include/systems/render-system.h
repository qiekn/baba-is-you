#pragma once

#include "managers/texture-manager.h"
#include "types.h"

class RenderSystem {
public:
  RenderSystem(Registry& registry, TextureManager& texture_manager)
      : registry_(registry), texture_manager_(texture_manager) {}

  virtual ~RenderSystem() = default;

  void DrawScene();
  void DrawUI();

private:
  // scene
  void DrawBackground();
  void DrawGrid();
  void DrawEntities();

  // ui
  void DrawImGui();

  // helper functions

private:
  Registry& registry_;
  TextureManager& texture_manager_;

  /**
   * @brief index for current texture2d (sprite animation)
   */
  int index_ = 0;
  float timer_ = 0;
  float duration_ = 0.2f;
};
