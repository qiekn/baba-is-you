cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_MODULE_STD OFF)

project(baba LANGUAGES C CXX)

# Default to Release for single-config generators when caller didn't specify
# CMAKE_BUILD_TYPE (suppresses raylib's default-build warning too).
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libraries as shared" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/raylib/CMakeLists.txt")
  message(FATAL_ERROR
    "raylib submodule is missing. Run: git submodule update --init --recursive")
endif()

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui.h")
  message(FATAL_ERROR
    "imgui submodule is missing. Run: git submodule update --init --recursive")
endif()

foreach(_dep IN ITEMS
    "deps/entt/single_include/entt/entt.hpp"
    "deps/json/single_include/nlohmann/json.hpp"
    "deps/magic-enum/include/magic_enum/magic_enum.hpp")
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_dep}")
    message(FATAL_ERROR
      "Submodule header missing: ${_dep}. "
      "Run: git submodule update --init --recursive")
  endif()
endforeach()

add_subdirectory(deps/raylib)

add_library(imgui STATIC
  deps/imgui/imgui.cpp
  deps/imgui/imgui_draw.cpp
  deps/imgui/imgui_tables.cpp
  deps/imgui/imgui_widgets.cpp
  deps/imgui/imgui_demo.cpp
  deps/imgui/backends/imgui_impl_glfw.cpp
  deps/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC deps/imgui deps/imgui/backends)
target_include_directories(imgui PRIVATE deps/raylib/src/external/glfw/include)

add_executable(${PROJECT_NAME}
  src/board.cpp
  src/game.cpp
  src/game_layer.cpp
  src/ids.cpp
  src/imgui_layer.cpp
  src/layer_stack.cpp
  src/level.cpp
  src/main.cpp
  src/rule_engine.cpp
  src/sprite_sheet.cpp
  src/undo_stack.cpp
  src/world.cpp
)

# User-facing executable name; the CMake target is still `baba` so scripts and
# generators that reference the target stay readable. Output: "Baba Is You.exe"
# on Windows, "Baba Is You" elsewhere.
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Baba Is You")

# Hide the Windows console window. CMake translates WIN32_EXECUTABLE to
# -mwindows on Clang/MinGW and /SUBSYSTEM:WINDOWS on MSVC; no-op on Linux/Mac.
# The MinGW/MSYS CRT supplies a WinMain shim that calls our main(), so no
# source change is required. Run with WIN32_EXECUTABLE=OFF if you need a
# console for debugging.
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

# On Linux releases we ship the libc++/libunwind runtime next to the
# executable, so embed a relative rpath that points there. Harmless when
# those libs aren't present (loader falls back to the system path).
if(UNIX AND NOT APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
  )
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    deps/entt/single_include
    deps/json/single_include
    deps/magic-enum/include
    deps/raygui
    deps/raylib/src/external/glfw/include
    src
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    raylib
    imgui
)

if (APPLE)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      "-framework IOKit"
      "-framework Cocoa"
      "-framework OpenGL"
  )
endif()
