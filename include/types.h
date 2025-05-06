#pragma once

#include <cctype>
#include <string>

struct Vector2Int {
  int x;
  int y;

  Vector2Int() : x(0), y(0) {}

  Vector2Int(int _x, int _y) : x(_x), y(_y) {}

  Vector2Int operator+(const Vector2Int &rhs) const {
    return Vector2Int(x + rhs.x, y + rhs.y);
  }

  Vector2Int operator-(const Vector2Int &rhs) const {
    return Vector2Int(x - rhs.x, y - rhs.y);
  }

  Vector2Int operator*(int rhs) const { return Vector2Int(x * rhs, y * rhs); }

  Vector2Int operator/(int rhs) const { return Vector2Int(x / rhs, y / rhs); }

  bool operator==(const Vector2Int &rhs) const {
    return x == rhs.x && y == rhs.y;
  }

  Vector2Int &operator+=(const Vector2Int &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }

  Vector2Int &operator-=(const Vector2Int &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
};

inline std::string ToLower(const std::string &str) {
  std::string res = str;
  for (char &c : res) {
    c = std::tolower(c);
  }
  return res;
}
