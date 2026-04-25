#pragma once

#include "ids.h"

struct Cell {
  int x = 0;
  int y = 0;
};

struct ObjectBlock {
  ObjectId id;
};

struct TextBlock {
  TextId id;
};

struct Facing {
  Direction dir = Direction::Right;
};

struct AnimFrame {
  int frame = 1;  // 1..3
  float t = 0.0f;
};

struct IsYou {};
struct IsWin {};
struct IsStop {};
struct IsPush {};
struct IsMove {};
struct IsDefeat {};
