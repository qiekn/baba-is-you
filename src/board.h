#pragma once

#include <optional>
#include <utility>

#include <raylib.h>

namespace board {

// Mutable so the game can resize the board to match whatever level is loaded.
inline int kCols = 24;
inline int kRows = 18;
inline constexpr float kBoardPadding = 28.0f;
inline constexpr float kBoardOffsetY = 14.0f;
inline constexpr float kBoardVerticalMargin = 30.0f;
inline constexpr float kGridLineThickness = 1.0f;

// Pixel size of a single cell for the currently loaded board, sized so the
// board fills the available window minus padding. Recomputed each call.
float Pitch();

// Screen-space rectangle that the board occupies (not including padding).
Rectangle BoardRect();

// Screen-space rectangle for the interior of one cell — what a sprite should
// fill. Accepts out-of-range indices and clamps; callers should sanity-check
// first.
Rectangle CellRect(int col, int row);

// Maps a screen-space point to a cell index. Returns nullopt if the point
// lies outside the board grid.
std::optional<std::pair<int, int>> ScreenToCell(Vector2 p);

}  // namespace board
