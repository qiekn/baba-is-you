// Copyright (c) 2020-2023 Chris Ohk

#pragma once

#include "enums/game_enums.h"
#include "games/object.h"

#include <string_view>
#include <vector>

using Position = std::pair<std::size_t, std::size_t>;

class Map {
public:
  Map() = default;

  Map(std::size_t width, std::size_t height);

  void Reset();

  std::size_t GetWidth() const;

  std::size_t GetHeight() const;

  void Load(std::string_view filename);

  void AddObject(std::size_t x, std::size_t y, ObjectType type);

  // Removes an object from the map.
  void RemoveObject(std::size_t x, std::size_t y, ObjectType type);

  // Assigns an object to the map.
  Object &At(std::size_t x, std::size_t y);

  // Assigns an object to the map.
  const Object &At(std::size_t x, std::size_t y) const;

  // Gets a list of icon positions.
  std::vector<Position> GetPositions(ObjectType type) const;

private:
  bool IsBoundary(std::size_t x, std::size_t y) const;

  std::size_t width_ = 0;
  std::size_t height_ = 0;

  std::vector<Object> initObjects_;
  std::vector<Object> objects_;
};
