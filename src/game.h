#pragma once

struct Game {
  void Run();

private:
  void Init();
  void Tick();
  void Update();
  void Render();
  void Shutdown();

  static constexpr int kScreenWidth = 1280;
  static constexpr int kScreenHeight = 720;
  static constexpr int kTargetFps = 60;
};