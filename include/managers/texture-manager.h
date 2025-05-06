#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

class TextureManager {
public:
  TextureManager() {}

  virtual ~TextureManager() { UnloadAll(); }

  std::vector<Texture2D>& GetFrames(const std::string& name);

  void UnloadAll();

private:
  std::unordered_map<std::string, std::vector<Texture2D>> frames_map_;
};
