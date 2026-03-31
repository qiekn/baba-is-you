module;

#include <raylib.h>
#include <string>

export module baba.utils;

export import baba.object;
export import baba.types;

export std::string GetSpritePrefixName(ObjectType type);
export Color Haxc(const std::string& color);
