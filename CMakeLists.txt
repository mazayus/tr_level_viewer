# TR Level Viewer

cmake_minimum_required(VERSION 3.0)

project(tr_level_viewer)

add_executable(tr_level_viewer
    code/camera.cpp
    code/main.cpp
    code/renderer.cpp
    code/shaders.cpp
    code/tr_loader.cpp
    code/tr_types.cpp
)

target_compile_options(tr_level_viewer PUBLIC
    -std=c++11 -pedantic -Wall -Wextra
    -Wno-unused-parameter
    -fno-strict-aliasing
)

target_link_libraries(tr_level_viewer
    SDL2
    GL
)
