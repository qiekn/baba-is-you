#include "board.h"

#include <algorithm>

namespace board {

namespace {
int LayoutColsForPitch() {
  return kScaleToLevel ? kCols : std::max(kCols, kBaseCols);
}

int LayoutRowsForPitch() {
  return kScaleToLevel ? kRows : std::max(kRows, kBaseRows);
}

int DisplayCols() {
  return kCols;
}

int DisplayRows() {
  return kRows;
}

int ViewportW() {
  return kViewportWidth > 0 ? kViewportWidth : std::max(1, GetScreenWidth());
}
int ViewportH() {
  return kViewportHeight > 0 ? kViewportHeight : std::max(1, GetScreenHeight());
}
}  // namespace

void SetViewportSize(int w, int h) {
  kViewportWidth = w;
  kViewportHeight = h;
}

void SetViewportOrigin(Vector2 origin) {
  kViewportOrigin = origin;
}

float Pitch() {
  const float avail_w = ViewportW() - 2 * kBoardPadding;
  const float avail_h = ViewportH() - 2 * (kBoardPadding + kBoardVerticalMargin);
  return std::min(avail_w / std::max(LayoutColsForPitch(), 1),
                  avail_h / std::max(LayoutRowsForPitch(), 1));
}

Rectangle BoardRect() {
  const float pitch = Pitch();
  const float w = DisplayCols() * pitch;
  const float h = DisplayRows() * pitch;
  return {
      (ViewportW() - w) * 0.5f,
      (ViewportH() - h) * 0.5f + kBoardOffsetY,
      w,
      h,
  };
}

int RenderCols() { return DisplayCols(); }

int RenderRows() { return DisplayRows(); }

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
