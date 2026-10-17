cmake_minimum_required(VERSION 3.22)
project(dota_item_merge_android LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PLATFORM Android CACHE STRING "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)
FetchContent_MakeAvailable(raylib)

include(../../cmake/Box2D.cmake)
add_library(merge_physics STATIC ../../src/game.c)
target_include_directories(merge_physics PUBLIC ../../src)
target_compile_options(merge_physics PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(merge_physics PUBLIC box2d)

add_library(dota-item-merge SHARED ../../src/main.c)
target_include_directories(dota-item-merge PRIVATE
    ../../src "${ANDROID_NDK}/sources/android/native_app_glue")
target_compile_definitions(dota-item-merge PRIVATE PLATFORM_ANDROID)
target_compile_options(dota-item-merge PRIVATE -Wall -Wextra)
target_link_libraries(dota-item-merge PRIVATE merge_physics raylib android log EGL GLESv2 m)
# Keep the NativeActivity entry point from raylib's static native-app glue.
# 16 KB alignment supports Android devices with larger memory pages.
target_link_options(dota-item-merge PRIVATE
    "-Wl,-u,ANativeActivity_onCreate" "-Wl,-z,max-page-size=16384")
