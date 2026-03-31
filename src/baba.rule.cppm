module;

#include <cstddef>
#include <string>
#include <string_view>
#include "entt.h"
#include "magic-enum.h"

export module baba.rule;

export import baba.object;

export {
#include "games/rule.h"
}
