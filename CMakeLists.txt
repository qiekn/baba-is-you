cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

project(game)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# dependencies

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libraries as shared" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/raylib)

add_subdirectory(deps/imgui)
add_subdirectory(deps/rlimgui)

# -----------------------------------------------------------------------------
# project

add_compile_definitions(ASSETS="${CMAKE_CURRENT_LIST_DIR}/assets")
add_compile_definitions(LEVELS="${CMAKE_CURRENT_LIST_DIR}/levels")

set(HDRS
    include/types.h
    include/constants.h
    include/maid.h
    include/components/basic-comp.h
    include/components/game-comp.h
    include/entities/prefabs.h
    include/enums/game-enums.h
    include/enums/object-enums.h
    include/enums/rule-enums.h
    include/games/command.h
    include/games/game.h
    include/games/object.h
    include/games/rule.h
    include/games/utils.h
    include/managers/color-manager.h
    include/managers/command-manager.h
    include/managers/input-manager.h
    include/managers/level-manager.h
    include/managers/rule-manager.h
    include/managers/scene-manager.h
    include/managers/texture-manager.h
    include/scenes/game-scene.h
    include/scenes/logo-scene.h
    include/scenes/menu-scene.h
    include/scenes/pause-scene.h
    include/scenes/scene.h
    include/scenes/title-scene.h
    include/systems/move-system.h
    include/systems/render-system.h
)

set(SRCS
    src/entities/prefabs.cpp
    src/games/commands.cpp
    src/games/game.cpp
    src/games/rule.cpp
    src/games/utils.cpp
    src/main.cpp
    src/managers/color-manager.cpp
    src/managers/command-manager.cpp
    src/managers/input-manager.cpp
    src/managers/level-manager.cpp
    src/managers/rule-manager.cpp
    src/managers/texture-manager.cpp
    src/scenes/game-scene.cpp
    src/scenes/logo-scene.cpp
    src/scenes/menu-scene.cpp
    src/scenes/pause-scene.cpp
    src/scenes/title-scene.cpp
    src/systems/move-system.cpp
    src/systems/render-system.cpp
    src/systems/scene-manager.cpp
)

set(MODULE_SRCS
    src/baba.types.cppm
    src/baba.object.cppm
    src/baba.rule.cppm
    src/baba.utils.cppm
    src/baba.base.cppm
    src/baba.cppm
)

# targets
add_executable(${PROJECT_NAME} ${SRCS})

target_sources(${PROJECT_NAME}
  PRIVATE
    FILE_SET CXX_MODULES FILES ${MODULE_SRCS}
)

set_source_files_properties(
  ${MODULE_SRCS}
  PROPERTIES
    COMPILE_OPTIONS "-O0"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    include/defs
    deps/entt
    deps/json
    deps/magic-enum
)

# libraries
target_link_libraries(${PROJECT_NAME}
    raylib rlimgui
)

# checks if OSX and links appropriate frameworks (only required on macOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

# format
add_custom_target(format 
    COMMAND clang-format -i ${SRCS} ${HDRS}
    COMMENT "Running clang-format on all header & cpp files"
)
