#include "board.h"

namespace board {

Rectangle BoardRect() {
  const float w = kCols * kCellPitch;
  const float h = kRows * kCellPitch;
  return {
      (GetScreenWidth() - w) * 0.5f,
      (GetScreenHeight() - h) * 0.5f + kBoardOffsetY,
      w,
      h,
  };
}

Rectangle CellRect(int col, int row) {
  const Rectangle b = BoardRect();
  const float inset = (kCellPitch - kCellInnerSize) * 0.5f;
  return {
      b.x + col * kCellPitch + inset,
      b.y + row * kCellPitch + inset,
      kCellInnerSize,
      kCellInnerSize,
  };
}

std::optional<std::pair<int, int>> ScreenToCell(Vector2 p) {
  const Rectangle b = BoardRect();
  if (p.x < b.x || p.y < b.y) return std::nullopt;
  const int col = static_cast<int>((p.x - b.x) / kCellPitch);
  const int row = static_cast<int>((p.y - b.y) / kCellPitch);
  if (col < 0 || col >= kCols || row < 0 || row >= kRows) return std::nullopt;
  return std::make_pair(col, row);
}

}  // namespace board
