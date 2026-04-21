#include "ids.h"

namespace {

constexpr std::string_view kObjectNames[kObjectCount] = {
    "baba", "flag", "wall", "rock", "grass", "flower", "tile", "cloud", "star",
};

constexpr std::string_view kTextNames[kTextCount] = {
    "is", "and", "not", "baba",  "flag", "wall", "rock",
    "you", "win", "stop", "push", "move", "defeat",
};

constexpr std::string_view kDirectionNames[4] = {
    "right",
    "up",
    "left",
    "down",
};

}  // namespace

std::string_view NameOf(ObjectId id) { return kObjectNames[static_cast<int>(id)]; }
std::string_view NameOf(TextId id) { return kTextNames[static_cast<int>(id)]; }
std::string_view NameOf(Direction dir) { return kDirectionNames[static_cast<int>(dir)]; }

TextCategory CategoryOf(TextId id) {
  switch (id) {
    case TextId::Is:
      return TextCategory::Verb;
    case TextId::And:
      return TextCategory::Conjunction;
    case TextId::Not:
      return TextCategory::Modifier;
    case TextId::Baba:
    case TextId::Flag:
    case TextId::Wall:
    case TextId::Rock:
      return TextCategory::Noun;
    case TextId::You:
    case TextId::Win:
    case TextId::Stop:
    case TextId::Push:
    case TextId::Move:
    case TextId::Defeat:
      return TextCategory::Property;
    case TextId::kCount:
      break;
  }
  return TextCategory::Property;  // unreachable
}

std::optional<ObjectId> NounToObject(TextId id) {
  switch (id) {
    case TextId::Baba:
      return ObjectId::Baba;
    case TextId::Flag:
      return ObjectId::Flag;
    case TextId::Wall:
      return ObjectId::Wall;
    case TextId::Rock:
      return ObjectId::Rock;
    default:
      return std::nullopt;
  }
}

DrawLayer LayerOf(ObjectId id) {
  switch (id) {
    case ObjectId::Grass:
    case ObjectId::Flower:
    case ObjectId::Tile:
      return DrawLayer::Floor;
    case ObjectId::Cloud:
    case ObjectId::Star:
      return DrawLayer::Float;
    case ObjectId::Baba:
    case ObjectId::Flag:
    case ObjectId::Wall:
    case ObjectId::Rock:
    case ObjectId::kCount:
      break;
  }
  return DrawLayer::Object;
}

std::optional<ObjectId> ObjectIdFromName(std::string_view name) {
  for (int i = 0; i < kObjectCount; ++i) {
    if (kObjectNames[i] == name) return static_cast<ObjectId>(i);
  }
  return std::nullopt;
}

std::optional<TextId> TextIdFromName(std::string_view name) {
  for (int i = 0; i < kTextCount; ++i) {
    if (kTextNames[i] == name) return static_cast<TextId>(i);
  }
  return std::nullopt;
}
