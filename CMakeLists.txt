cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_MODULE_STD OFF)

project(baba LANGUAGES C CXX)

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

add_subdirectory(deps/raylib)
add_subdirectory(deps/imgui)
add_subdirectory(deps/rlimgui)

add_executable(${PROJECT_NAME}
  src/game.cpp
  src/main.cpp
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    deps/entt
    deps/json
    deps/magic-enum
    deps/raygui
    src
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    raylib
    imgui
    rlimgui
)

if (APPLE)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      "-framework IOKit"
      "-framework Cocoa"
      "-framework OpenGL"
  )
endif()