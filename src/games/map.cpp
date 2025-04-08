// Copyright (c) 2020-2023 Chris Ohk

#include "games/map.h"
#include <fstream>

Map::Map(std::size_t width, std::size_t height)
    : width_(width), height_(height) {
  initObjects_.reserve(width_ * height_);
  objects_.reserve(width_ * height_);

  for (std::size_t i = 0; i < width_ * height_; ++i) {
    initObjects_.emplace_back(std::vector<ObjectType>{ObjectType::ICON_EMPTY});
    objects_.emplace_back(std::vector<ObjectType>{ObjectType::ICON_EMPTY});
  }
}

void Map::Reset() { objects_ = initObjects_; }

std::size_t Map::GetWidth() const { return width_; }

std::size_t Map::GetHeight() const { return height_; }

void Map::Load(std::string_view filename) {
  std::ifstream mapFile(filename.data());

  mapFile >> width_ >> height_;

  int val = 0;
  for (std::size_t i = 0; i < width_ * height_; ++i) {
    mapFile >> val;

    initObjects_.emplace_back(
        std::vector<ObjectType>{static_cast<ObjectType>(val)});
    objects_.emplace_back(
        std::vector<ObjectType>{static_cast<ObjectType>(val)});
  }
}

void Map::AddObject(std::size_t x, std::size_t y, ObjectType type) {
  objects_.at(y * width_ + x).Add(type, IsBoundary(x, y));
}

void Map::RemoveObject(std::size_t x, std::size_t y, ObjectType type) {
  objects_.at(y * width_ + x).Remove(type);
}

Object &Map::At(std::size_t x, std::size_t y) {
  return objects_.at(y * width_ + x);
}

const Object &Map::At(std::size_t x, std::size_t y) const {
  return objects_.at(y * width_ + x);
}

std::vector<Position> Map::GetPositions(ObjectType type) const {
  std::vector<Position> res;

  for (std::size_t y = 0; y < height_; ++y) {
    for (std::size_t x = 0; x < width_; ++x) {
      if (At(x, y).HasType(type)) {
        res.emplace_back(std::make_pair(x, y));
      }
    }
  }

  return res;
}

bool Map::IsBoundary(std::size_t x, std::size_t y) const {
  return x == 0 || x == width_ - 1 || y == 0 || y == height_ - 1;
}
