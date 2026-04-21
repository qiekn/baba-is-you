#pragma once

#include <array>
#include <filesystem>

#include <raylib.h>

#include "ids.h"

class SpriteSheet {
 public:
  static constexpr int kFrameCount = 3;
  static constexpr int kVariantCount = 16;

  SpriteSheet() = default;
  ~SpriteSheet() { Unload(); }

  SpriteSheet(const SpriteSheet&) = delete;
  SpriteSheet& operator=(const SpriteSheet&) = delete;

  // Loads PNGs from `sprites_dir`. For every ObjectId / TextId we read frame
  // 1..3. Auto-tiled objects additionally read variants 1..15. Returns true
  // iff every expected file loaded successfully.
  bool LoadAll(const std::filesystem::path& sprites_dir);
  void Unload();

  // frame is 1-based (1, 2, 3) to match on-disk naming. variant is the 4-bit
  // neighbor mask (0..15); ignored for non-tiled objects. Out-of-range inputs
  // are clamped.
  const Texture2D& Get(ObjectId id, int frame, int variant = 0) const;
  const Texture2D& Get(TextId id, int frame) const;

  Color TintFor(ObjectId id) const;
  Color TintFor(TextId id) const;

 private:
  // objects_[object][variant][frame]. Non-tiled objects only populate
  // variant 0; the rest stay at Texture2D{}.
  std::array<std::array<std::array<Texture2D, kFrameCount>, kVariantCount>, kObjectCount> objects_{};
  std::array<std::array<Texture2D, kFrameCount>, kTextCount> texts_{};
  bool loaded_ = false;
};
