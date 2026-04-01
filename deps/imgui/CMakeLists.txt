add_library(imgui STATIC
   imgui.cpp
   imgui_draw.cpp
   imgui_tables.cpp
   imgui_widgets.cpp
   imgui_demo.cpp
)

target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
