#include "managers/level-manager.h"
#include <raylib.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "components/basic-comp.h"
#include "constants.h"
#include "entities/prefabs.h"
#include "json.h"

LevelManager::LevelManager(Registry& registry, Prefabs& prefabs)
    : registry_(registry), prefabs_(prefabs) {}

LevelManager::~LevelManager() {}

void LevelManager::LoadJson() {
  TraceLog(LOG_DEBUG, "LevelManager: Load Json");
  std::filesystem::path path = kLevels / "demo.json";
  std::ifstream file(path);
  if (!file.is_open()) {
    TraceLog(LOG_ERROR, "failed to open level file: %s", path.c_str());
    return;
  }

  nlohmann::json json_data;
  file >> json_data;

  levels_.clear();
  level_index_map_.clear();

  if (json_data.contains("levels") && json_data["levels"].is_array()) {
    for (const auto& level_json : json_data["levels"]) {
      LevelData level;
      // level id
      if (level_json.contains("id") && level_json["id"].is_number_integer()) {
        level.id = level_json["id"];
      } else {
        TraceLog(LOG_ERROR, "load level id wrong, \n%s",
                 level_json.is_string());
      }

      // level name
      if (level_json.contains("name") && level_json["name"].is_string()) {
        level.name = level_json["name"].get<std::string>();
      } else {
        TraceLog(LOG_ERROR, "load level name wrong, \n%s",
                 level_json.is_string());
      }

      // level description
      if (level_json.contains("description") &&
          level_json["description"].is_string()) {
        level.description = level_json["description"].get<std::string>();
      } else {
        TraceLog(LOG_ERROR, "load level description wrong, \n%s",
                 level_json.is_string());
      }

      // level tiles
      if (level_json.contains("tiles") && level_json["tiles"].is_array()) {
        for (const auto& tile_json : level_json["tiles"]) {
          if (tile_json.size() >= 4) {
            LevelTile tile;
            tile.x = tile_json["x"].get<int>();
            tile.y = tile_json["y"].get<int>();
            tile.z = tile_json["z"].get<int>();
            tile.name = tile_json["name"].get<std::string>();
            level.tiles.push_back(tile);
          }
        }
      }

      levels_.push_back(level);
      level_index_map_[level.name] = level.id;
    }
  }
  TraceLog(LOG_INFO, "load %d levels", levels_.size());
}

void LevelManager::InitLevel() {
  if (levels_.empty() || current_level_ < 0 ||
      current_level_ >= levels_.size()) {
    TraceLog(LOG_ERROR, "level index invalid!");
    return;
  }

  const LevelData& data = levels_[current_level_];
  TraceLog(LOG_INFO, "load level %s", data.name.c_str());

  for (const auto& tile : data.tiles) {
    auto pos = Position{tile.x, tile.y};
    prefabs_.CreateText(pos, tile.name);
  }
}

void LevelManager::NextLevel() { JumpLevel(++current_level_); }

void LevelManager::PrevLevel() { JumpLevel(--current_level_); }

void LevelManager::JumpLevel(int index) {
  if (index >= 0 && index < levels_.size()) {
    current_level_ = index;
  } else {
    TraceLog(LOG_ERROR, "level index: %d is out of range!", index);
  }
  InitLevel();
}
