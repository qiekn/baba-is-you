# RlImGui (raylib backend for imgui)

add_library(rlimgui STATIC rlimgui.cpp)

target_include_directories(rlimgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(rlimgui PUBLIC imgui PRIVATE raylib)
