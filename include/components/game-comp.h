#pragma once

#include "games/object.h"

struct Tile {
  ObjectType type;
};

// core
struct Subjct {};
struct Verb {};
struct Predicate {};
struct Icon {};

// properties
#define X(a) \
  struct IS_##a {};
#include "properties.def"
#undef X
