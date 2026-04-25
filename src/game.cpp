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
  SetTraceLogLevel(LOG_WARNING);
  const WindowState state = LoadWindowState(kScreenWidth, kScreenHeight);

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
  InitWindow(state.w, state.h, "baba is you");
  SetWindowPosition(state.x, state.y);
  SetWindowIconFromSvg("assets/icons/favicon.svg");
  SetTargetFPS(kTargetFps);
  InitAudioDevice();

  // Initialize layers

  auto imgui_layer = std::make_unique<ImGuiLayer>();
  auto game_layer = std::make_unique<GameLayer>();

  imgui_layer_ = imgui_layer.get();
  game_layer_ = game_layer.get();

  layers_.PushLayer(std::move(game_layer));
  layers_.PushOverlay(std::move(imgui_layer));
}

void Game::Tick() {
  Update();
  Render();
}

void Game::Update() {
  const float dt = GetFrameTime();
  if (IsKeyPressed(KEY_F11)) {
    ToggleBorderless();
  }
  for (auto& layer : layers_) {
    layer->OnUpdate(dt);
  }
}

void Game::ToggleBorderless() {
  if (!borderless_) {
    windowed_pos_ = GetWindowPosition();
    windowed_size_ = {(float)GetScreenWidth(), (float)GetScreenHeight()};

    const int monitor = GetCurrentMonitor();
    const Vector2 mpos = GetMonitorPosition(monitor);
    const int mw = GetMonitorWidth(monitor);
    const int mh = GetMonitorHeight(monitor);

    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition((int)mpos.x, (int)mpos.y);
    // +1 px so Windows doesn't auto-promote this to exclusive fullscreen
    // (WS_POPUP + exact-monitor-size triggers fullscreen optimizations).
    SetWindowSize(mw, mh + 1);
    borderless_ = true;
  } else {
    ClearWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowSize((int)windowed_size_.x, (int)windowed_size_.y);
    SetWindowPosition((int)windowed_pos_.x, (int)windowed_pos_.y);
    borderless_ = false;
  }
}

void Game::Render() {
  BeginDrawing();
  const auto edge_bg = game_layer_ ? game_layer_->LevelEdge() : std::nullopt;
  ClearBackground(edge_bg.value_or(imgui_layer_->BackgroundColor()));

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
  if (borderless_) ToggleBorderless();
  SaveWindowState();
  layers_.Clear();  // detach layers before the GL context goes away
  CloseAudioDevice();
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

  // Imported levels override the board interior with their palette color so
  // the themed background reads correctly even though our ImGui theme is
  // still active.
  if (game_layer_) {
    if (auto bg = game_layer_->LevelBackground()) {
      DrawRectangleRounded(board_background, 0.04f, 10, *bg);
    }
  }

  if (!imgui_layer_->ShowGrid()) return;

  const Color border = imgui_layer_->BoardGridBorderColor();
  const float t = board::kGridLineThickness;
  const float pitch = board::Pitch();

  // Single-line grid: each interior boundary is drawn once, so adjacent cells
  // share an edge instead of floating inside inset rectangles.
  for (int col = 0; col <= board::kCols; ++col) {
    const float x = board.x + col * pitch - t * 0.5f;
    DrawRectangleRec({x, board.y, t, board.height}, border);
  }
  for (int row = 0; row <= board::kRows; ++row) {
    const float y = board.y + row * pitch - t * 0.5f;
    DrawRectangleRec({board.x, y, board.width, t}, border);
  }
}
