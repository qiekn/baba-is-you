module;

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <list>
#include <memory>
#include <raylib.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "entt.h"
#include "magic-enum.h"

export module baba;

export import baba.base;
export import baba.utils;

export {
#include "enums/game-enums.h"
#include "enums/rule-enums.h"

#include "components/basic-comp.h"
#include "components/game-comp.h"

#include "systems/move-system.h"

#include "games/command.h"

#include "entities/prefabs.h"

#include "managers/color-manager.h"
#include "managers/command-manager.h"
#include "managers/input-manager.h"
#include "managers/level-manager.h"
#include "managers/rule-manager.h"
#include "managers/texture-manager.h"

#include "systems/render-system.h"

#include "scenes/scene.h"
#include "scenes/logo-scene.h"
#include "scenes/menu-scene.h"
#include "scenes/game-scene.h"
#include "scenes/pause-scene.h"
#include "scenes/title-scene.h"

#include "managers/scene-manager.h"

#include "maid.h"
#include "games/game.h"
}
