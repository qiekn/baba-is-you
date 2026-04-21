# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.


## Project

A from-scratch remake of *Baba Is You* in C++23. Currently a scaffold: raylib window + ImGui editor panel that renders a themed grid board. There is no game logic yet — `Game::Update()` is empty.

原版游戏在 ./original-baba-is-you/

## Build & Run

First-time setup requires the git submodules (raylib, imgui):

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja
cmake --build build
./build/baba
```

`run.sh` is the common dev shortcut (rebuild + run):

```sh
./run.sh
```

The binary resolves assets relative to the CWD (e.g. `assets/fonts/opensans/OpenSans-Regular.ttf`), so **always run from the repo root**, not from `build/`.

There are no tests or lint targets configured. Formatting is enforced via the Google-based `.clang-format` (2-space indent, 120 col, `PointerAlignment: Left`).

## Toolchain Requirements

- CMake **≥ 3.30** (set in `CMakeLists.txt`)
- C++23 with `-stdlib=libc++` — the project hardcodes libc++, so Clang is required (not GCC/MSVC).
- `CMAKE_CXX_MODULE_STD` is **OFF**; do not `import std;`. Use classic `#include` throughout.
- `cmake/EnableCxxImportStd.cmake` exists but is **not** included by `CMakeLists.txt`; it's a stashed helper for future module-std work.

## Architecture

Single executable, two translation units:

- `src/main.cpp` — thin entry point, instantiates `Game` and calls `Run()`.
- `src/game.{h,cpp}` — entire runtime lives here.

`Game::Run()` is the canonical lifecycle: `Init()` → loop(`Tick()` = `Update()` + `Render()`) → `Shutdown()`. raylib owns the window/event loop; ImGui is bridged through rlImGui (`rlImGuiBegin/End` wraps raylib's `BeginDrawing/EndDrawing`). When adding new game state, keep it as members on `Game` and hook into `Update`/`Render` — do not create parallel lifecycles.

The board is a constant grid (`kBoardCols × kBoardRows` at `kCellPitch` px) drawn every frame from `DrawGridBackground()`. Theme presets live in the `kThemes` static array; `ApplyTheme()` copies into the mutable `*_color_` members that `Render()` reads. The ImGui `DrawEditorPanel()` lets you live-edit those colors.

## Dependencies (all in `deps/`)

Built via CMake subdirs:
- `raylib` — submodule (forked at `qiekn/raylib`), built as static lib
- `imgui` — submodule on the **docking** branch; compiled as a local static `imgui` target from `CMakeLists.txt` (core + draw + tables + widgets + demo)
- `rlimgui` — vendored CMake subdir bridging imgui ↔ raylib

Header-only, wired via `target_include_directories` only:
- `entt` (ECS), `json` (nlohmann), `magic-enum`, `raygui`

When adding `#include`s for the header-only libs, just include the header directly (e.g. `#include <entt/entt.hpp>`) — they're already on the include path for the `baba` target.

## Repo Layout Notes

- `build/` — gitignored build output.
- `original-baba-is-you/` — untracked local copy of the reference game; not part of the build.
- `docs/` — unrelated mdBook template scaffold, not project documentation.
- `main` is the PR target branch; active work happens on `remake`.
