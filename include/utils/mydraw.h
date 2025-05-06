#pragma once

#include "constants.h"
#include <raylib.h>
#include <sstream>
#include <string>
#include <vector>

const float FRAME_DURATION = 0.2f;
const std::string SPRITE_PATH = "assets/sprites/";

struct AnimatedSprite {
  int index; // current frame index
  float timer;
  float duration; // frame duration
  std::vector<Texture2D> frames;

  AnimatedSprite(const std::string &name, int count = 3,
                 float _duration = FRAME_DURATION) {
    duration = _duration;
    timer = 0.0f;
    index = 0;

    // load all frames
    for (int i = 1; i <= count; i++) {
      // generate path
      std::stringstream path;
      path << SPRITE_PATH << "text_" << name << "_0_" << i << ".png";
      std::string filepath = path.str();

      // load texture
      Image image = LoadImage(filepath.c_str());
      Texture2D texture = LoadTextureFromImage(image);
      UnloadImage(image);
      if (texture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: %s", filepath.c_str());
      }
      frames.push_back(texture);

      // check texture size 24x24
      if (texture.width != 24 || texture.height != 24) {
        TraceLog(LOG_WARNING, "Texture %s is not 24x24 (size: %dx%d)",
                 filepath.c_str(), texture.width, texture.height);
        UnloadTexture(texture);
        continue;
      }
      // check frames not empty
      if (frames.empty()) {
        TraceLog(LOG_ERROR, "No valid textures loaded for sprite: %s",
                 name.c_str());
      }
    }
  }

  ~AnimatedSprite() {
    for (auto &texture : frames) {
      UnloadTexture(texture);
    }
  }

  void Update(float delta_time) {
    if (frames.empty())
      return;
    if (!kAnimated)
      return;
    timer += delta_time;
    if (timer >= duration) {
      timer -= duration;
      index = (index + 1) % frames.size();
    }
  }

  void Draw(int x, int y) {
    Texture2D texture = GetCurrentFrame();
    DrawTextureEx(texture, Vector2(x, y), 0, kScale, PINK);
    Update(GetFrameTime());
  }

  Texture2D GetCurrentFrame() const { return frames[index]; }
};
