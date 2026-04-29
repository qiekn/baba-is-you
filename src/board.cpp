#include "board.h"

#include <algorithm>

namespace board {

namespace {
int DisplayCols() {
  return kScaleToLevel ? kCols : std::max(kCols, kBaseCols);
}

int DisplayRows() {
  return kScaleToLevel ? kRows : std::max(kRows, kBaseRows);
}

int OffsetX() {
  return (DisplayCols() - kCols) / 2;
}

int OffsetY() {
  return (DisplayRows() - kRows) / 2;
}
}  // namespace

float Pitch() {
  const float avail_w = GetScreenWidth() - 2 * kBoardPadding;
  const float avail_h = GetScreenHeight() - 2 * (kBoardPadding + kBoardVerticalMargin);
  return std::min(avail_w / std::max(DisplayCols(), 1),
                  avail_h / std::max(DisplayRows(), 1));
}

Rectangle BoardRect() {
  const float pitch = Pitch();
  const float w = DisplayCols() * pitch;
  const float h = DisplayRows() * pitch;
  return {
      (GetScreenWidth() - w) * 0.5f,
      (GetScreenHeight() - h) * 0.5f + kBoardOffsetY,
      w,
      h,
  };
}

int RenderCols() { return DisplayCols(); }

int RenderRows() { return DisplayRows(); }

Rectangle CellRect(int col, int row) {
  const Rectangle b = BoardRect();
  const float pitch = Pitch();
  const int ox = OffsetX();
  const int oy = OffsetY();
  return {
      b.x + (col + ox) * pitch,
      b.y + (row + oy) * pitch,
      pitch,
      pitch,
  };
}

std::optional<std::pair<int, int>> ScreenToCell(Vector2 p) {
  const Rectangle b = BoardRect();
  const float pitch = Pitch();
  if (p.x < b.x || p.y < b.y) return std::nullopt;
  const int ox = OffsetX();
  const int oy = OffsetY();
  const int col = static_cast<int>((p.x - b.x) / pitch) - ox;
  const int row = static_cast<int>((p.y - b.y) / pitch) - oy;
  if (col < 0 || col >= kCols || row < 0 || row >= kRows) return std::nullopt;
  return std::make_pair(col, row);
}

}  // namespace board
