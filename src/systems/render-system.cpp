#include "systems/render-system.h"
#include <entt.h>
#include <raylib.h>
#include "components/components.h"
#include "constants.h"
#include "rlimgui.h"

// public

void RenderSystem::DrawScene() {
  DrawBackground();
  DrawGrid();
  DrawEntities();
}

void RenderSystem::DrawUI() {
  DrawImGui();

  /* draw screen area
  int border = 4;
  DrawText("SCREEN AREA", kScreenWidth - 160, 10, 20, RED);
  DrawRectangle(0, 0, kScreenWidth, border, RED);
  DrawRectangle(0, border, border, kScreenHeight - 10, RED);
  DrawRectangle(kScreenWidth - border, border, border, kScreenHeight - 10, RED);
  DrawRectangle(0, kScreenHeight - border, kScreenWidth, border, RED);
  */
}

// private

void RenderSystem::DrawBackground() {
  auto background_outer = Color{38, 30, 67, 255};
  auto background_inner = Color{26, 19, 35, 255};
  ClearBackground(background_outer);
  auto height = kCellSize * kRows * kScale;
  auto width = kCellSize * kCols * kScale;
  DrawRectangle(0, 0, width, height, background_inner);
}

void RenderSystem::DrawGrid() {
  int sz = kCellSize * kScale;
  Color color = Color{47, 34, 86, 255};

  // horizontal lines
  for (int i = 0; i <= kRows; i++) {
    DrawLine(0, i * sz, kCols * sz, i * sz, color);
  }

  // vertical lines
  for (int i = 0; i <= kCols; i++) {
    DrawLine(i * sz, 0, i * sz, kRows * sz, color);
  }
}

void RenderSystem::DrawEntities() {
  // make sure all animated sprite has AnimationState component
  InitAnimations();

  UpdateAnimations();

  auto view = registry_.view<Position, SpriteRenderer>(entt::exclude<Hide>);
  for (auto entity : view) {
    const auto& pos = view.get<Position>(entity);
    const auto& render = view.get<SpriteRenderer>(entity);
    float px = pos.x * kCellSize * kScale;
    float py = pos.y * kCellSize * kScale;

    auto& frames = texture_manager_.GetFrames(render.name);

    int current_frame = 0;

    // update current index of frame_texture
    if (render.is_animated && registry_.all_of<AnimationState>(entity)) {
      const auto& anim = registry_.get<AnimationState>(entity);
      current_frame = anim.frame_index;
    }

    Texture2D current_texture = frames[current_frame];
    DrawTextureEx(current_texture, Vector2(px, py), 0, kScale, PINK);
  }
}

// helpers

void RenderSystem::DrawImGui() {
  rlImGuiBegin();

  bool open = true;
  // ImGui::ShowDemoWindow(&open);

  rlImGuiEnd();
}

void RenderSystem::InitAnimations() {
  auto view = registry_.view<SpriteRenderer>(entt::exclude<AnimationState>);
  for (auto entity : view) {
    const auto& renderer = view.get<SpriteRenderer>(entity);
    if (renderer.is_animated) {
      registry_.emplace<AnimationState>(entity);
    }
  }
};

void RenderSystem::UpdateAnimations() {
  float delta_time = GetFrameTime();

  auto view = registry_.view<SpriteRenderer, AnimationState>();
  for (auto entity : view) {
    auto& render = view.get<SpriteRenderer>(entity);
    auto& anim = view.get<AnimationState>(entity);

    if (render.is_animated) {
      auto frames = texture_manager_.GetFrames(render.name);
      if (!frames.empty()) {
        anim.timer += delta_time;
        if (anim.timer >= anim.frame_duration) {
          anim.timer -= anim.frame_duration;
          anim.frame_index = (anim.frame_index + 1) % frames.size();
        }
      }
    }
  }
};
