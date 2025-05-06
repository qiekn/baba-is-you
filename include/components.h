#pragma once

#include "types.h"

// Properties
struct Position : public Vector2Int {
  Position() : Vector2Int() {}
  Position(int x, int y) : Vector2Int(x, y) {}
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
