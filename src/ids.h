#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

enum class ObjectId : std::uint8_t {
  Baba,
  Flag,
  Wall,
  Rock,
  Grass,
  Flower,
  Tile,
  Cloud,
  Star,
  kCount,
};

enum class TextId : std::uint8_t {
  Is,
  And,
  Not,
  Baba,
  Flag,
  Wall,
  Rock,
  You,
  Win,
  Stop,
  Push,
  Move,
  Defeat,
  kCount,
};

enum class Direction : std::uint8_t {
  Right,
  Up,
  Left,
  Down,
};

enum class TextCategory : std::uint8_t {
  Noun,
  Verb,
  Conjunction,
  Modifier,
  Property,
};

// Render order, low to high. Only used for draw sorting — rules apply
// regardless of layer, but we have no text blocks for Floor/Float kinds so
// they never become subjects in practice.
enum class DrawLayer : std::uint8_t {
  Floor,
  Object,
  Float,
};

inline constexpr int kObjectCount = static_cast<int>(ObjectId::kCount);
inline constexpr int kTextCount = static_cast<int>(TextId::kCount);

// Lowercase base name used for asset filenames.
// Objects:  "baba"      -> assets/sprites/baba_0_{frame}.png
// Text:     "baba"      -> assets/sprites/text_baba_0_{frame}.png
std::string_view NameOf(ObjectId id);
std::string_view NameOf(TextId id);
std::string_view NameOf(Direction dir);

TextCategory CategoryOf(TextId id);
DrawLayer LayerOf(ObjectId id);

// True if the object uses 16-variant auto-tiling (wall, brick, etc.). When
// true, SpriteSheet loads wall_{0..15}_{frame}.png and the renderer picks the
// variant based on same-kind neighbors.
bool IsAutoTiled(ObjectId id);

// Noun text -> its corresponding object. Non-noun text returns nullopt.
std::optional<ObjectId> NounToObject(TextId id);

// Lookup by lowercase name. Returns nullopt on miss.
std::optional<ObjectId> ObjectIdFromName(std::string_view name);
std::optional<TextId> TextIdFromName(std::string_view name);
