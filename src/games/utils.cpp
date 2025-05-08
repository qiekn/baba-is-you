#include "games/utils.h"
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
