cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(game)

#---------------------------------------------------------------------3
#                            Dependencies                             |
#---------------------------------------------------------------------3

# Raylib
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()

add_subdirectory(deps/imgui)
add_subdirectory(deps/rlimgui)
add_subdirectory(deps/entt)
add_subdirectory(deps/json)

#---------------------------------------------------------------------3
#                             Our Project                             |
#---------------------------------------------------------------------3

add_compile_definitions(ASSETS="${CMAKE_CURRENT_LIST_DIR}/assets")
add_compile_definitions(LEVELS="${CMAKE_CURRENT_LIST_DIR}/levels")

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

# headers
include_directories(include)

file(GLOB_RECURSE HDRS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/*.h"
)

# sources
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# targets
add_executable(${PROJECT_NAME} ${SRCS})

# libraries
target_link_libraries(${PROJECT_NAME}
    raylib rlimgui
    entt
    json
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
