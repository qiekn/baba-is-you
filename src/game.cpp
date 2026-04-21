#include "game.h"

#include <raylib.h>
#include <rlgl.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include <array>
#include <cstdio>
#include <memory>
#include <vector>

#include "board.h"
#include "game_layer.h"

namespace {
constexpr const char* kWindowStateFile = "window.state";

struct WindowState {
  int x;
  int y;
  int w;
  int h;
};

WindowState LoadWindowState(int default_w, int default_h) {
  WindowState s{80, 80, default_w, default_h};
  if (FILE* f = std::fopen(kWindowStateFile, "r")) {
    int x, y, w, h;
    if (std::fscanf(f, "%d %d %d %d", &x, &y, &w, &h) == 4 && w > 0 && h > 0) {
      s = {x, y, w, h};
    }
    std::fclose(f);
  }
  return s;
}

void SaveWindowState() {
  Vector2 pos = GetWindowPosition();
  if (FILE* f = std::fopen(kWindowStateFile, "w")) {
    std::fprintf(f, "%d %d %d %d\n", (int)pos.x, (int)pos.y, GetScreenWidth(), GetScreenHeight());
    std::fclose(f);
  }
}

void SetWindowIconFromSvg(const char* path) {
  NSVGimage* svg = nsvgParseFromFile(path, "px", 96.0f);
  if (!svg || svg->width <= 0.0f || svg->height <= 0.0f) {
    if (svg) nsvgDelete(svg);
    return;
  }

  NSVGrasterizer* rast = nsvgCreateRasterizer();
  if (!rast) {
    nsvgDelete(svg);
    return;
  }

  constexpr int kSizes[] = {16, 32, 48, 64, 128, 256};
  constexpr int kCount = (int)(sizeof(kSizes) / sizeof(kSizes[0]));

  std::array<std::vector<unsigned char>, kCount> buffers;
  std::array<Image, kCount> images;

  for (int i = 0; i < kCount; ++i) {
    const int size = kSizes[i];
    buffers[i].assign((size_t)size * size * 4, 0);
    const float scale = (float)size / svg->width;
    nsvgRasterize(rast, svg, 0.0f, 0.0f, scale, buffers[i].data(), size, size, size * 4);
    images[i] = Image{
        .data = buffers[i].data(),
        .width = size,
        .height = size,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
  }
  SetWindowIcons(images.data(), kCount);

  nsvgDeleteRasterizer(rast);
  nsvgDelete(svg);
}
}  // namespace

void Game::Run() {
  Init();

  while (!WindowShouldClose()) {
    Tick();
  }

  Shutdown();
}

void Game::Init() {
  const WindowState state = LoadWindowState(kScreenWidth, kScreenHeight);

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
  InitWindow(state.w, state.h, "baba");
  SetWindowPosition(state.x, state.y);
  SetWindowIconFromSvg("assets/icons/favicon.svg");
  SetTargetFPS(kTargetFps);

  auto imgui_layer = std::make_unique<ImGuiLayer>();
  imgui_layer_ = imgui_layer.get();
  layers_.PushLayer(std::make_unique<GameLayer>());
  layers_.PushOverlay(std::move(imgui_layer));
}

void Game::Tick() {
  Update();
  Render();
}

void Game::Update() {
  const float dt = GetFrameTime();
  for (auto& layer : layers_) {
    layer->OnUpdate(dt);
  }
}

void Game::Render() {
  BeginDrawing();
  ClearBackground(imgui_layer_->BackgroundColor());

  DrawGridBackground();
  for (auto& layer : layers_) {
    layer->OnRender();
  }
  rlDrawRenderBatchActive();

  imgui_layer_->Begin();
  if (imgui_layer_->IsVisible()) {
    for (auto& layer : layers_) {
      layer->OnImGuiRender();
    }
  }
  imgui_layer_->End();

  EndDrawing();
}

void Game::Shutdown() {
  SaveWindowState();
  layers_.Clear();  // detach layers before the GL context goes away
  CloseWindow();
}

void Game::DrawGridBackground() const {
  const Rectangle board = board::BoardRect();
  const Rectangle board_background = {
      board.x - board::kBoardPadding,
      board.y - board::kBoardPadding,
      board.width + board::kBoardPadding * 2.0f,
      board.height + board::kBoardPadding * 2.0f,
  };

  DrawRectangleRounded(board_background, 0.04f, 10, imgui_layer_->BoardBackgroundColor());

  const Color border = imgui_layer_->BoardGridBorderColor();
  const float cell_roundness = 0.18f;

  for (int row = 0; row < board::kRows; ++row) {
    for (int col = 0; col < board::kCols; ++col) {
      DrawRectangleRoundedLinesEx(board::CellRect(col, row), cell_roundness, 8, 2.5f, border);
    }
  }
}
