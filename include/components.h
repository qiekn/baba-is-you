#pragma once

#include <string>
#include "types.h"

// Properties

struct Position : public Vector2Int {
  Position() : Vector2Int() {}
  Position(int x, int y) : Vector2Int(x, y) {}
};

enum class SpriteType {
  kNoun,
  kOperator,
  kProperty,
  kIcon,
};

struct SpriteRenderer {
  SpriteType type;
  std::string name;
  size_t frames = 3;
  bool is_animated = true;
};

// basic crates

struct Player {};

struct Wall {};

struct Target {};

// Tags

struct Movable {};

struct Imovable {};

/**
 * @class Hide
 * @brief ignored by render system
 *
 */
struct Hide {};
