#include "sprite_sheet.h"

#include <algorithm>
#include <string>

namespace {

Texture2D LoadSpritePixelArt(const std::filesystem::path& path) {
  Image img = LoadImage(path.string().c_str());
  if (img.data == nullptr) {
    return Texture2D{};
  }
  // Sprites ship as white-on-black RGB. Reinterpret each pixel's luminance as
  // alpha and force RGB to white; that way DrawTexturePro's tint parameter
  // multiplies directly into the visible color without the black background
  // bleeding through at the edges.
  ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  auto* bytes = static_cast<unsigned char*>(img.data);
  const int count = img.width * img.height;
  for (int i = 0; i < count; ++i) {
    const unsigned char r = bytes[i * 4 + 0];
    const unsigned char g = bytes[i * 4 + 1];
    const unsigned char b = bytes[i * 4 + 2];
    const unsigned char lum = static_cast<unsigned char>((r + g + b) / 3);
    bytes[i * 4 + 0] = 255;
    bytes[i * 4 + 1] = 255;
    bytes[i * 4 + 2] = 255;
    bytes[i * 4 + 3] = lum;
  }

  Texture2D tex = LoadTextureFromImage(img);
  UnloadImage(img);
  if (tex.id != 0) {
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
  }
  return tex;
}

void UnloadIfValid(Texture2D& tex) {
  if (tex.id != 0) {
    UnloadTexture(tex);
    tex.id = 0;
  }
}

constexpr Color kObjectTints[kObjectCount] = {
    {255, 255, 255, 255},  // Baba
    {237, 226, 133, 255},  // Flag
    {115, 115, 115, 255},  // Wall
    {186, 140, 102, 255},  // Rock
};

constexpr Color kTextTints[kTextCount] = {
    {255, 255, 255, 255},  // Is
    {255, 255, 255, 255},  // And
    {201, 56, 55, 255},    // Not
    {217, 57, 106, 255},   // Baba (text)
    {237, 226, 133, 255},  // Flag (text)
    {158, 158, 158, 255},  // Wall (text)
    {186, 140, 102, 255},  // Rock (text)
    {217, 57, 106, 255},   // You
    {255, 211, 113, 255},  // Win
    {75, 122, 72, 255},    // Stop
    {186, 140, 102, 255},  // Push
    {151, 208, 134, 255},  // Move
    {201, 56, 55, 255},    // Defeat
};

}  // namespace

bool SpriteSheet::LoadAll(const std::filesystem::path& sprites_dir) {
  Unload();
  bool ok = true;

  for (int i = 0; i < kObjectCount; ++i) {
    const std::string name{NameOf(static_cast<ObjectId>(i))};
    for (int f = 0; f < kFrameCount; ++f) {
      const std::filesystem::path p = sprites_dir / (name + "_0_" + std::to_string(f + 1) + ".png");
      objects_[i][f] = LoadSpritePixelArt(p);
      if (objects_[i][f].id == 0) ok = false;
    }
  }

  for (int i = 0; i < kTextCount; ++i) {
    const std::string name = "text_" + std::string{NameOf(static_cast<TextId>(i))};
    for (int f = 0; f < kFrameCount; ++f) {
      const std::filesystem::path p = sprites_dir / (name + "_0_" + std::to_string(f + 1) + ".png");
      texts_[i][f] = LoadSpritePixelArt(p);
      if (texts_[i][f].id == 0) ok = false;
    }
  }

  loaded_ = ok;
  return ok;
}

void SpriteSheet::Unload() {
  for (auto& row : objects_)
    for (auto& tex : row) UnloadIfValid(tex);
  for (auto& row : texts_)
    for (auto& tex : row) UnloadIfValid(tex);
  loaded_ = false;
}

const Texture2D& SpriteSheet::Get(ObjectId id, int frame) const {
  const int f = std::clamp(frame, 1, kFrameCount) - 1;
  return objects_[static_cast<int>(id)][f];
}

const Texture2D& SpriteSheet::Get(TextId id, int frame) const {
  const int f = std::clamp(frame, 1, kFrameCount) - 1;
  return texts_[static_cast<int>(id)][f];
}

Color SpriteSheet::TintFor(ObjectId id) const { return kObjectTints[static_cast<int>(id)]; }
Color SpriteSheet::TintFor(TextId id) const { return kTextTints[static_cast<int>(id)]; }
