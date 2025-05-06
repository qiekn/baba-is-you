#include "systems/render-system.h"
#include <entt.h>
#include <raylib.h>
#include "components.h"
#include "constants.h"
#include "imgui.h"
#include "rlimgui.h"

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
  auto view = registry_.view<Position, SpriteRenderer>(entt::exclude<Hide>);
  for (auto entity : view) {
    const auto& pos = view.get<Position>(entity);
    const auto& render = view.get<SpriteRenderer>(entity);
    float px = pos.x * kCellSize;
    float py = pos.y * kCellSize;

    auto frames = texture_manager_.GetFrames(render.name);

    // update current index of frame_texture
    if (render.is_animated) {
      timer_ += GetFrameTime();
      if (timer_ >= duration_) {
        timer_ -= duration_;
        index_ = (index_ + 1) % frames.size();
      }
    }

    Texture2D current_texture = frames[index_];
    DrawTextureEx(current_texture, Vector2(px, py), 0, kScale, PINK);
  }
}

void RenderSystem::DrawImGui() {
  rlImGuiBegin();

  bool open = true;
  // ImGui::ShowDemoWindow(&open);

  rlImGuiEnd();
}
