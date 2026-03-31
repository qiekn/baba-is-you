# Repository Guidelines

## Project Structure & Module Organization
Core gameplay code lives in `src/`, split by domain: `games/`, `systems/`, `managers/`, `scenes/`, and `entities/`. Public headers mirror that layout under `include/`. Shared definitions used by enums and generated-style macros are in `include/defs/`. Runtime content is stored in `assets/` and level data in `levels/`. Third-party dependencies live in `deps/`; `raylib`, `imgui`, and `rlimgui` are built through CMake, while header-only libraries such as `entt`, `json`, and `magic-enum` are included directly.

## Build, Test, and Development Commands
Configure once with:

```bash
cmake -S . -B build
```

Build the game with:

```bash
cmake --build build -j
```

Run the executable from the build directory:

```bash
./build/game
```

The repo also provides `run.sh`, which builds and launches the game, or opens `gdb` with `./run.sh debug`. Format all tracked C++ sources and headers with:

```bash
cmake --build build --target format
```

## Coding Style & Naming Conventions
This project uses C++20 and `.clang-format` based on Google style with an 80-column limit. Keep indentation consistent with the formatter and prefer small, focused translation units. Match the existing file naming style: lowercase, hyphen-separated headers and sources such as `render-system.cpp` and `level-manager.h`. Types use PascalCase, while functions and variables use lower_snake_case unless an external API dictates otherwise.

## Testing Guidelines
There is no dedicated automated test suite yet. For changes, at minimum:
1. Reconfigure if CMake changed.
2. Rebuild successfully.
3. Run `./build/game` and verify the affected scene, rule, or input flow manually.

If you add test coverage later, place it under a top-level `tests/` directory and wire it into CMake explicitly.

## Commit & Pull Request Guidelines
Follow the existing prefix-based commit style, for example `chore: update run.sh`. Prefer concise subjects like `fix: resolve rule parsing bug` or `feat: add pause scene input`. Pull requests should describe gameplay impact, list build/manual verification steps, and include screenshots or short clips for visible UI or rendering changes.
