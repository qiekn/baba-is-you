#include <ostream>

struct Vector2Int {
  int x;
  int y;

  Vector2Int() : x(0), y(0) {}
  Vector2Int(int x, int y) : x(x), y(y) {}

  Vector2Int operator+(const Vector2Int &other) const {
    return Vector2Int(x + other.x, y + other.y);
  }

  Vector2Int operator-(const Vector2Int &other) const {
    return Vector2Int(x - other.x, y - other.y);
  }

  bool operator==(const Vector2Int &other) const {
    return x == other.x && y == other.y;
  }

  bool operator!=(const Vector2Int &other) const { return !(*this == other); }

  friend std::ostream &operator<<(std::ostream &os, const Vector2Int &v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
  }
};
