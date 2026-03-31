module;

#include <cctype>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include "entt.h"

export module baba.types;

export struct Vector2Int {
  int x;
  int y;

  Vector2Int() : x(0), y(0) {}

  Vector2Int(int _x, int _y) : x(_x), y(_y) {}

  Vector2Int operator+(const Vector2Int& rhs) const {
    return Vector2Int(x + rhs.x, y + rhs.y);
  }

  Vector2Int operator-(const Vector2Int& rhs) const {
    return Vector2Int(x - rhs.x, y - rhs.y);
  }

  Vector2Int operator*(int rhs) const { return Vector2Int(x * rhs, y * rhs); }

  Vector2Int operator/(int rhs) const { return Vector2Int(x / rhs, y / rhs); }

  bool operator==(const Vector2Int& rhs) const {
    return x == rhs.x && y == rhs.y;
  }

  Vector2Int& operator+=(const Vector2Int& rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }

  Vector2Int& operator-=(const Vector2Int& rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
};

export inline std::string ToLower(const std::string& str) {
  std::string res = str;
  for (char& c : res) {
    c = std::tolower(c);
  }
  return res;
}

export using Registry = entt::registry;
export using Dispatcher = entt::dispatcher;
export using Entity = entt::entity;

template <>
struct std::hash<Vector2Int> {
  std::size_t operator()(const Vector2Int& v) const {
    return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
  }
};

export using Map = std::unordered_map<Vector2Int, Entity, std::hash<Vector2Int>>;

export inline constexpr int KFps = 60;

export inline constexpr bool kAnimated = false;
export inline constexpr int kMaxUndoHistry = 99;

export inline constexpr int kCellSize = 24;
export inline constexpr int kScale = 2;
export inline constexpr int kRows = 18;
export inline constexpr int kCols = 22;
export inline constexpr int kCellsPerRow = kCols;
export inline constexpr int kCellsPerColumn = kRows;

export inline constexpr int kScreenBorder = 1 * kCellSize * kScale;
export inline constexpr int kScreenWidth =
    kCellSize * kCols * kScale + kScreenBorder;
export inline constexpr int kScreenHeight =
    kCellSize * kRows * kScale + kScreenBorder;

export inline const std::filesystem::path kAssets = ASSETS;
export inline const std::filesystem::path kLevels = LEVELS;
