#include "games/utils.h"
#include <raylib.h>
#include "games/object.h"
#include "magic-enum.h"
#include "types.h"

std::string GetSpritePrefixName(ObjectType type) {
  std::string_view enum_name = magic_enum::enum_name(type).substr(5);
  std::string name;
  if (IsText(type)) {
    name = std::string("text_") + ToLower(enum_name.data());
  } else if (IsIcon(type)) {
    name = ToLower(enum_name.data());
  }
  return name;
}

Color Haxc(const std::string& color) {
  if (color.size() != 7 || color[0] != '#') {
    TraceLog(LOG_ERROR, "Invalid color format: %s", color.c_str());
    return WHITE;
  }

  try {
    // Convert hex string (without #) to integer
    unsigned int num = std::stoul(color.substr(1), nullptr, 16);
    // Use Raylib's GetColor
    return GetColor(num << 8 | 0xFF);  // Shift to RRGGBBAA format
  } catch (const std::exception& e) {
    TraceLog(LOG_ERROR, "Invalid hex color: %s", color.c_str());
    return WHITE;
  }
}
