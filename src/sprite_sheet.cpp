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

  // Decide where the silhouette mask comes from. The original game ships
  // sprites in two flavours and stb_image normalises both to RGBA:
  //   1. RGB + tRNS chunk — stb expands it to RGBA, so alpha already encodes
  //      the silhouette and RGB keeps grayscale shading inside the body
  //      (e.g. baba's eyes are darker than the body fill so the tint
  //      multiplication produces a darker pupil).
  //   2. Plain RGB white-on-black — no transparency info, alpha is uniformly
  //      255 after format conversion. Fall back to luminance and force RGB
  //      to white so the tint can paint a clean shape without black bleed.
  unsigned char alpha_min = 255, alpha_max = 0;
  for (int i = 0; i < count; ++i) {
    const unsigned char a = bytes[i * 4 + 3];
    alpha_min = std::min(alpha_min, a);
    alpha_max = std::max(alpha_max, a);
  }
  const bool alpha_carries_mask = (alpha_max > alpha_min);
  if (!alpha_carries_mask) {
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
  }
  // Otherwise leave the data intact: alpha is already the mask and the
  // grayscale RGB will multiply correctly against the tint.

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
