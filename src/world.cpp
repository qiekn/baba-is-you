#include "world.h"

#include <fstream>

#include <nlohmann/json.hpp>

using nlohmann::json;

bool LoadWorld(const std::filesystem::path& path, World& out) {
  std::ifstream in(path);
  if (!in) return false;
  json doc;
  try {
    in >> doc;
  } catch (const json::exception&) {
    return false;
  }

  World w;
  w.title = doc.value("title", "");
  if (auto it = doc.find("levels"); it != doc.end() && it->is_array()) {
    for (const auto& node : *it) {
      WorldLevel lvl;
      lvl.id = node.value("id", "");
      lvl.name = node.value("name", node.value("label", lvl.id));
      if (!lvl.id.empty()) w.levels.push_back(std::move(lvl));
    }
  }
  out = std::move(w);
  return true;
}

bool LoadProgress(const std::filesystem::path& path, Progress& out) {
  std::ifstream in(path);
  if (!in) return false;
  json doc;
  try {
    in >> doc;
  } catch (const json::exception&) {
    return false;
  }
  Progress p;
  if (auto it = doc.find("completed"); it != doc.end() && it->is_array()) {
    for (const auto& node : *it) {
      if (node.is_string()) p.completed.insert(node.get<std::string>());
    }
  }
  out = std::move(p);
  return true;
}

bool SaveProgress(const std::filesystem::path& path, const Progress& progress) {
  json doc;
  json arr = json::array();
  for (const auto& id : progress.completed) arr.push_back(id);
  doc["completed"] = std::move(arr);
  std::ofstream out(path);
  if (!out) return false;
  out << doc.dump(2);
  return out.good();
}
