#include "systems/grid.h"

int Grid::cell_size_ = 24;
float Grid::cell_gap_ = 0;

Vector2Int Grid::WorldToCell(Vector2 pos) {
  int x = pos.x / (cell_size_ + cell_gap_);
  int y = pos.y / (cell_size_ + cell_gap_);
  return {x, y};
}

Vector2 Grid::Cell2World(Vector2Int pos) {
  float x = pos.x * (cell_size_ + cell_gap_);
  float y = pos.y * (cell_size_ + cell_gap_);
  return {x, y};
}
