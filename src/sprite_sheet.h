#pragma once

#include <array>
#include <filesystem>

#include <raylib.h>

#include "ids.h"

class SpriteSheet {
 public:
  static constexpr int kFrameCount = 3;

  SpriteSheet() = default;
  ~SpriteSheet() { Unload(); }

  SpriteSheet(const SpriteSheet&) = delete;
  SpriteSheet& operator=(const SpriteSheet&) = delete;

  // Loads the 17 * 3 = 51 PNGs from `sprites_dir`. Returns true iff every
  // expected file loaded successfully.
  bool LoadAll(const std::filesystem::path& sprites_dir);
  void Unload();

  // frame is 1-based (1, 2, 3) to match on-disk naming. Values outside are
  // clamped to the 1..3 range.
  const Texture2D& Get(ObjectId id, int frame) const;
  const Texture2D& Get(TextId id, int frame) const;

  Color TintFor(ObjectId id) const;
  Color TintFor(TextId id) const;

 private:
  std::array<std::array<Texture2D, kFrameCount>, kObjectCount> objects_{};
  std::array<std::array<Texture2D, kFrameCount>, kTextCount> texts_{};
  bool loaded_ = false;
};
