#pragma once

#include "entities/prefabs.h"
#include "managers/level-manager.h"
#include "managers/rule-manager.h"
#include "managers/texture-manager.h"
#include "systems/move-system.h"
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
  Prefabs prefabs_;

  /* managers */
  TextureManager texture_manager_;
  LevelManager level_manager_;
  RuleManager rule_manager_;

  /* systems */
  RenderSystem render_system_;
  MoveSystem move_system_;

private:
  Maid()
      : prefabs_(registry_),
        rule_manager_(registry_),
        level_manager_(registry_, prefabs_),
        render_system_(registry_, texture_manager_),
        move_system_(registry_) {
    TraceLog(LOG_DEBUG, "Maid is ready!");
  }
};
