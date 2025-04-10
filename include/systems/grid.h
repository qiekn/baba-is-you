#pragma once

#include "common.h"
#include <raylib.h>

class Grid {
public:
  static Vector2Int WorldToCell(Vector2 pos);
  static Vector2 Cell2World(Vector2Int pos);
  static void SetCellSize(int size) { cell_size_ = size; }
  static void SetCellGap(int gap) { cell_gap_ = gap; }

private:
  static int cell_size_;
  static float cell_gap_;
};
