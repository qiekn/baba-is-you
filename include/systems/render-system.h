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

  /**
   * @brief if entity has SpriteRenderer and is_animated is true, but don't have
   * AnimationState component, then add it.
   */
  void InitAnimations();

  void UpdateAnimations();

private:
  Registry& registry_;
  TextureManager& texture_manager_;
};
