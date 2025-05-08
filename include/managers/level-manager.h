#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "entities/prefabs.h"
#include "games/object.h"
#include "types.h"

struct LevelTile {
  int x, y, z;
  ObjectType type;
};

struct LevelData {
  int id;
  std::string name;
  std::string description;
  std::vector<LevelTile> tiles;
};

class LevelManager {
public:
  LevelManager(Registry& registry, Prefabs& prefabs);
  virtual ~LevelManager();

  void LoadJson();

  void InitLevel();
  void NextLevel();
  void PrevLevel();
  void JumpLevel(int index);

private:
  int current_level_ = 0;
  std::vector<LevelData> levels_;
  std::unordered_map<std::string, int> level_index_map_;
  Registry& registry_;
  Prefabs& prefabs_;
};
