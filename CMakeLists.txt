cmake_minimum_required(VERSION 3.30)
project(game)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

file(GLOB_RECURSE HDRS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/*.h"
)

# sources
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# targets
add_executable(${PROJECT_NAME} ${SRCS})

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
