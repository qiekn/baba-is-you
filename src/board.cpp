#include "board.h"

#include <algorithm>

namespace board {

float Pitch() {
  const float avail_w = GetScreenWidth() - 2 * kBoardPadding;
  const float avail_h = GetScreenHeight() - 2 * kBoardPadding - kBoardOffsetY;
  return std::min(avail_w / std::max(kCols, 1),
                  avail_h / std::max(kRows, 1));
}

Rectangle BoardRect() {
  const float pitch = Pitch();
  const float w = kCols * pitch;
  const float h = kRows * pitch;
  return {
      (GetScreenWidth() - w) * 0.5f,
      (GetScreenHeight() - h) * 0.5f + kBoardOffsetY,
      w,
      h,
  };
}

Rectangle CellRect(int col, int row) {
  const Rectangle b = BoardRect();
  const float pitch = Pitch();
  return {
      b.x + col * pitch,
      b.y + row * pitch,
      pitch,
      pitch,
  };
}

std::optional<std::pair<int, int>> ScreenToCell(Vector2 p) {
  const Rectangle b = BoardRect();
  const float pitch = Pitch();
  if (p.x < b.x || p.y < b.y) return std::nullopt;
  const int col = static_cast<int>((p.x - b.x) / pitch);
  const int row = static_cast<int>((p.y - b.y) / pitch);
  if (col < 0 || col >= kCols || row < 0 || row >= kRows) return std::nullopt;
  return std::make_pair(col, row);
}

}  // namespace board
