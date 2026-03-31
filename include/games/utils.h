#pragma once

#include <raylib.h>
#include <string>

// NONU_WALL -> text_wall
// ICON_WALL -> wall
std::string GetSpritePrefixName(ObjectType type);

// convert ##3b250c -> raylib color
Color Haxc(const std::string& color);
