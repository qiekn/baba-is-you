#include "managers/texture-manager.h"
#include <raylib.h>
#include <filesystem>
#include <string>
#include <utility>
#include "constants.h"

std::vector<Texture2D>& TextureManager::GetFrames(const std::string& name) {
  auto it = frames_map_.find(name);

  if (it == frames_map_.end()) {
    std::vector<Texture2D> frames;
    int count = 3;
    for (int i = 0; i < count; i++) {
      std::filesystem::path path =
          kAssets / "sprites" /
          (name + "_0_" + std::to_string(i + 1) + ".png");  // text_ba_0_3.png
      Texture2D texture = LoadTexture(path.c_str());
      frames.push_back(texture);
    }
    frames_map_[name] = frames;

    auto [new_it, success] = frames_map_.emplace(name, std::move(frames));
    return new_it->second;
  }
  return it->second;
}

void TextureManager::UnloadAll() {
  for (auto& pair : frames_map_) {
    for (Texture2D& texture : pair.second) {
      UnloadTexture(texture);
    }
  }
  frames_map_.clear();
}
