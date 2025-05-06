#pragma once

#include "managers/texture-manager.h"
#include "systems/render-system.h"
#include "types.h"

/**
 * @class Maid
 * @brief 有什么事情尽管来问女仆小姐姐就好了
 */
class Maid {
public:
  /* singleton */
  static Maid& Instance() {
    static Maid instance;
    return instance;
  }

  /* entities */
  Registry registry_;
  Dispatcher dispatcher_;
  Map entity_map_;

  /* managers */
  TextureManager texture_manager_;

  /* systems */
  RenderSystem render_system_;

private:
  Maid() : render_system_(registry_, texture_manager_) {}
};
