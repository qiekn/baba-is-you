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
  int frame = 1;        // 1..3 idle wobble (b in baba_aa_b.png)
  int walk_phase = 0;   // 0..3 walk-cycle offset within a direction group
                        // (added to DirectionToVariant for the aa value)
  float t = 0.0f;       // wobble timer
  float since_step = 999.0f;  // seconds since the entity last moved; resets
                              // walk_phase to 0 after a brief idle gap so the
                              // character "settles" into the idle pose.
};

struct IsYou {};
struct IsWin {};
struct IsStop {};
struct IsPush {};
struct IsMove {};
struct IsDefeat {};
