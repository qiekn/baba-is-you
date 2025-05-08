#pragma once

#include <string>
#include "types.h"

// Properties

struct Position : public Vector2Int {
  Position() : Vector2Int() {}
  Position(int x, int y) : Vector2Int(x, y) {}
  Position(Vector2Int pos) : Vector2Int(pos) {}
};

enum class SpriteType {
  kNone,
  kNoun,
  kOperator,
  kProperty,
  kIcon,
};

struct SpriteRenderer {
  std::string name;
  size_t frames = 3;
  bool is_animated = true;
};

struct AnimationState {
  int frame_index = 0;
  float frame_duration = 0.2f;
  float timer = 0.0f;
};

// basic crates

struct Player {};

struct Wall {};

struct Target {};

// Tags

struct Movable {};

struct Imovable {};

// ignored by render system
struct Hide {};
