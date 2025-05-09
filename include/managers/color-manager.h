#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include "games/object.h"

class ColorManager {
public:
  ColorManager();
  virtual ~ColorManager();

  // Get color from an object type
  Color GetColor(ObjectType type) const;

  // Set color for an object type
  void SetColor(ObjectType type, const Color& color);

  // Draw ImGui UI for color editing
  void DrawColorEditor();

  // Save/Load color configuration
  bool SaveToFile(const std::string& filename);
  bool LoadFromFile(const std::string& filename);

private:
  void InitDefaultColors();

private:
  std::unordered_map<ObjectType, Color> color_map_;
  // ???
  std::unordered_map<ObjectType, bool> category_option_;
};
