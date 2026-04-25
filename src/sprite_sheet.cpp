#include "sprite_sheet.h"

#include <algorithm>
#include <string>

namespace {

Texture2D LoadSpritePixelArt(const std::filesystem::path& path) {
  // Quietly skip absent files. Directional characters legally have sparse
  // walk-phase coverage (e.g. baba ships variants 0,1,2,3,7 within each
  // direction group), so probing the full 0..31 range is expected to miss
  // some files — we don't want raylib to spam those as load warnings.
  if (!FileExists(path.string().c_str())) return Texture2D{};
  Image img = LoadImage(path.string().c_str());
  if (img.data == nullptr) {
    return Texture2D{};
  }
  ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  auto* bytes = static_cast<unsigned char*>(img.data);
  const int count = img.width * img.height;

  // Original sprites encode the silhouette in *two* ways depending on the
  // asset, and we have to satisfy both:
  //   - Characters (baba/keke/me): silhouette in alpha (RGB+tRNS expanded by
  //     stb_image), interior shading like darker eyes baked into the RGB
  //     grayscale. We must keep the body opaque or eyes punch holes.
  //   - Auto-tile pieces (wall_*): the structural rounded corner is encoded
  //     as pure-black RGB pixels with alpha left at 255 (only 1-3 pixels per
  //     corner sprite have alpha < 255). If we treated alpha as the mask, the
  //     rounded corner would render as a black square edge instead of fading
  //     to background.
  // Common rule that handles both: pure-black RGB ⇒ transparent; otherwise
  // keep the source alpha. The RGB grayscale stays intact so DrawTexturePro's
  // tint multiplies into the right shading (darker eyes, darker brick lines).
  for (int i = 0; i < count; ++i) {
    const unsigned char r = bytes[i * 4 + 0];
    const unsigned char g = bytes[i * 4 + 1];
    const unsigned char b = bytes[i * 4 + 2];
    if (r == 0 && g == 0 && b == 0) {
      bytes[i * 4 + 3] = 0;
    }
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
      // Directional sprites use 4 direction-bases (0/8/16/24) and within each
      // direction up to 8 walk-phase offsets. Load the full 0..31 range so
      // GameLayer can advance through the walk cycle when the entity steps.
      for (int v = 0; v < 32; ++v) load_variant(name, i, v);
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
