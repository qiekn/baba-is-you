#include "sprite_sheet.h"

#include <algorithm>
#include <string>

namespace {

Texture2D LoadSpritePixelArt(const std::filesystem::path& path) {
  Image img = LoadImage(path.string().c_str());
  if (img.data == nullptr) {
    return Texture2D{};
  }
  // Sprites ship in two formats: most frames are white-on-black RGB (no alpha
  // channel), where we want luminance to become the mask. A few PNGs (e.g.
  // baba_0_1.png) are RGBA with the silhouette baked into the alpha channel
  // and black RGB everywhere — for those, luminance is zero so we must read
  // the existing alpha instead. Detect the source format before normalizing.
  const bool source_has_alpha = (img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 ||
                                 img.format == PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
  ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  auto* bytes = static_cast<unsigned char*>(img.data);
  const int count = img.width * img.height;
  for (int i = 0; i < count; ++i) {
    const unsigned char r = bytes[i * 4 + 0];
    const unsigned char g = bytes[i * 4 + 1];
    const unsigned char b = bytes[i * 4 + 2];
    const unsigned char a = bytes[i * 4 + 3];
    const unsigned char lum = static_cast<unsigned char>((r + g + b) / 3);
    // Take whichever channel actually carries the silhouette so DrawTexturePro's
    // tint can multiply onto a clean white shape regardless of source encoding.
    const unsigned char mask = source_has_alpha ? std::max(lum, a) : lum;
    bytes[i * 4 + 0] = 255;
    bytes[i * 4 + 1] = 255;
    bytes[i * 4 + 2] = 255;
    bytes[i * 4 + 3] = mask;
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

}  // namespace

bool SpriteSheet::LoadAll(const std::filesystem::path& sprites_dir) {
  Unload();
  bool any_loaded = false;

  auto load_variant = [&](const std::string& name, int object_index, int v) {
    for (int f = 0; f < kFrameCount; ++f) {
      const std::filesystem::path p =
          sprites_dir / (name + "_" + std::to_string(v) + "_" + std::to_string(f + 1) + ".png");
      auto tex = LoadSpritePixelArt(p);
      if (tex.id != 0) any_loaded = true;
      objects_[object_index][v][f] = tex;
    }
  };

  for (int i = 0; i < kObjectCount; ++i) {
    const auto id = static_cast<ObjectId>(i);
    const std::string name{InfoOf(id).sprite};

    if (IsAutoTiled(id)) {
      for (int v = 0; v < 16; ++v) load_variant(name, i, v);
    } else if (IsDirectional(id)) {
      for (Direction d : {Direction::Right, Direction::Up, Direction::Left, Direction::Down}) {
        load_variant(name, i, DirectionToVariant(d));
      }
    } else {
      load_variant(name, i, 0);
    }
  }

  for (int i = 0; i < kTextCount; ++i) {
    // TextInfo::name is the on-disk basename — most are "text_<short>" but a
    // few (e.g. "default") drop the prefix.
    const std::string name{InfoOf(static_cast<TextId>(i)).name};
    for (int f = 0; f < kFrameCount; ++f) {
      const std::filesystem::path p = sprites_dir / (name + "_0_" + std::to_string(f + 1) + ".png");
      auto tex = LoadSpritePixelArt(p);
      if (tex.id != 0) any_loaded = true;
      texts_[i][f] = tex;
    }
  }

  loaded_ = any_loaded;
  return any_loaded;
}

void SpriteSheet::Unload() {
  for (auto& obj : objects_)
    for (auto& variant : obj)
      for (auto& tex : variant) UnloadIfValid(tex);
  for (auto& row : texts_)
    for (auto& tex : row) UnloadIfValid(tex);
  loaded_ = false;
}

const Texture2D& SpriteSheet::Get(ObjectId id, int frame, int variant) const {
  const int f = std::clamp(frame, 1, kFrameCount) - 1;
  const int v = std::clamp(variant, 0, kVariantCount - 1);
  return objects_[static_cast<int>(id)][v][f];
}

const Texture2D& SpriteSheet::Get(TextId id, int frame) const {
  const int f = std::clamp(frame, 1, kFrameCount) - 1;
  return texts_[static_cast<int>(id)][f];
}

Color SpriteSheet::TintFor(ObjectId id) const {
  const auto& info = InfoOf(id);
  return Color{info.tint_r, info.tint_g, info.tint_b, 255};
}

Color SpriteSheet::TintFor(TextId id) const {
  const auto& info = InfoOf(id);
  return Color{info.tint_r, info.tint_g, info.tint_b, 255};
}
