#pragma once

#include <optional>
#include <utility>

#include <raylib.h>

namespace board {

// Mutable so the game can resize the board to match whatever level is loaded.
inline int kCols = 24;
inline int kRows = 18;
inline constexpr int kBaseCols = 24;
inline constexpr int kBaseRows = 18;
inline bool kScaleToLevel = true;
inline constexpr float kBoardPadding = 28.0f;
inline constexpr float kBoardOffsetY = 14.0f;
inline constexpr float kBoardVerticalMargin = 30.0f;
inline float kGridLineThickness = 3.5f;

// Width/height of the area the board renders into. Set every frame by
// imgui_layer (matches the docked Viewport panel size); falls back to the OS
// window size when the UI is hidden.
inline int kViewportWidth = 0;
inline int kViewportHeight = 0;
// Top-left corner of the Viewport panel in OS-window coordinates. Used to
// translate raylib's GetMousePosition() into board-local space.
inline Vector2 kViewportOrigin{0.0f, 0.0f};
// True while the mouse is over the docked Viewport panel (or the UI is
// hidden, in which case the whole window is the viewport).
inline bool kViewportHovered = true;
void SetViewportSize(int w, int h);
void SetViewportOrigin(Vector2 origin);

// Pixel size of a single cell for the currently loaded board, sized so the
// board fills the available window minus padding. Recomputed each call.
float Pitch();

// Screen-space rectangle that the board occupies (not including padding).
Rectangle BoardRect();

// Grid dimensions used for rendering and hit-testing.
int RenderCols();
int RenderRows();

// Screen-space rectangle for the interior of one cell — what a sprite should
// fill. Accepts out-of-range indices and clamps; callers should sanity-check
// first.
Rectangle CellRect(int col, int row);

// Maps a screen-space point to a cell index. Returns nullopt if the point
// lies outside the board grid.
std::optional<std::pair<int, int>> ScreenToCell(Vector2 p);

}  // namespace board
