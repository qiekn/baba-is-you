#include "level.h"

#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

using nlohmann::json;

bool LoadLevelFromJson(const std::filesystem::path& path, Level& out) {
  std::ifstream in(path);
  if (!in) return false;

  json doc;
  try {
    in >> doc;
  } catch (const json::exception&) {
    return false;
  }

  Level level;
  level.cols = doc.value("cols", 24);
  level.rows = doc.value("rows", 18);
  level.name = doc.value("name", "");

  // Optional [r, g, b] background from the imported palette.
  if (auto it = doc.find("background"); it != doc.end() && it->is_array() && it->size() == 3) {
    level.bg_r = (*it)[0].get<int>();
    level.bg_g = (*it)[1].get<int>();
    level.bg_b = (*it)[2].get<int>();
  }
  if (auto it = doc.find("edge"); it != doc.end() && it->is_array() && it->size() == 3) {
    level.edge_r = (*it)[0].get<int>();
    level.edge_g = (*it)[1].get<int>();
    level.edge_b = (*it)[2].get<int>();
  }

  if (auto it = doc.find("tiles"); it != doc.end() && it->is_array()) {
    level.tiles.reserve(it->size());
    for (const auto& node : *it) {
      LevelTile tile;
      tile.x = node.value("x", 0);
      tile.y = node.value("y", 0);
      const std::string kind = node.value("kind", "object");
      const std::string name = node.value("name", "");
      if (kind == "object") {
        auto id = ObjectIdFromName(name);
        if (!id) continue;
        tile.kind = *id;
      } else if (kind == "text") {
        auto id = TextIdFromName(name);
        if (!id) continue;
        tile.kind = *id;
      } else {
        continue;
      }
      level.tiles.push_back(tile);
    }
  }

  out = std::move(level);
  return true;
}

bool SaveLevelToJson(const std::filesystem::path& path, const Level& level) {
  json doc;
  doc["cols"] = level.cols;
  doc["rows"] = level.rows;
  json tiles = json::array();
  for (const auto& tile : level.tiles) {
    json node;
    node["x"] = tile.x;
    node["y"] = tile.y;
    if (tile.IsObject()) {
      node["kind"] = "object";
      node["name"] = std::string{NameOf(std::get<ObjectId>(tile.kind))};
    } else {
      node["kind"] = "text";
      node["name"] = std::string{NameOf(std::get<TextId>(tile.kind))};
    }
    tiles.push_back(std::move(node));
  }
  doc["tiles"] = std::move(tiles);

  std::ofstream out(path);
  if (!out) return false;
  out << doc.dump(2);
  return out.good();
}
