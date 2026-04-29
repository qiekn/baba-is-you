#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

struct WorldLevel {
  std::string id;    // matches JSON filename stem, e.g. "001"
  std::string name;  // display text
};

struct World {
  std::string title;
  std::vector<WorldLevel> levels;
};

struct Progress {
  std::unordered_set<std::string> completed;
};

bool LoadWorld(const std::filesystem::path& path, World& out);
bool LoadProgress(const std::filesystem::path& path, Progress& out);
bool SaveProgress(const std::filesystem::path& path, const Progress& progress);
