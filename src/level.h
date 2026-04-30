#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include "ids.h"

struct LevelTile {
  int x = 0;
  int y = 0;
  std::variant<ObjectId, TextId> kind;
  int layer = 2;  // editor-authored layering bucket (1..3)

  bool IsObject() const { return std::holds_alternative<ObjectId>(kind); }
  bool IsText() const { return std::holds_alternative<TextId>(kind); }
};

struct Level {
  int cols = 24;
  int rows = 18;
  std::vector<LevelTile> tiles;

  // Optional display name (e.g. "where do i go?") for imported levels.
  std::string name;

  // Optional palette colours. -1 means "unset" — the caller should fall
  // back to its normal theme.
  // `bg_*`   = interior fill for the playfield (palette cell 0,4).
  // `edge_*` = outer fill around the playfield (palette cell 1,0).
  int bg_r = -1;
  int bg_g = -1;
  int bg_b = -1;
  int edge_r = -1;
  int edge_g = -1;
  int edge_b = -1;
};

// Both return false and leave `out` untouched on IO/parse failure.
bool LoadLevelFromJson(const std::filesystem::path& path, Level& out);
bool SaveLevelToJson(const std::filesystem::path& path, const Level& level);
