#pragma once

#include <array>
#include <filesystem>

#include <raylib.h>

#include "ids.h"

/**
 * @class SpriteSheet
 * @brief Hello, this is qiekn. Let me explain this class
 *
 * In this game baba is you, we have different sprites
 * for more, you can see: [Game Elements](https://www.spriters-resource.com/pc_computer/babaisyou/)
 *
 * 1. Tiled                - this is the map layer like grass, wall, cloud, fence
 * 2. Text                 - just a 3 frame animation
 * 3. Object (Static)      - just a 3 frame animation
 * 4. Object (Directional) - you can filter arrow_*.png to understand this
 * 5. Object (Character)   - the most complex, e.g. baba_xx_y.png
 *    xx is [0, 31] -> 8 * 4 = 32, [0, 7] is right, the next 8 is up, and then left, down
 *    y  is [1, 3] -> when we don't move, the sprites has a 3 frame animation
 */
class SpriteSheet {
 public:
  static constexpr int kFrameCount = 3;
  // Wide enough to hold both the 0..15 auto-tile bitmask range and the
  // directional slots (0, 8, 16, 24) used by characters like Baba.
  static constexpr int kVariantCount = 32;

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
