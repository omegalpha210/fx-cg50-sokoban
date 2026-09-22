cmake_minimum_required(VERSION 3.15)
project(Sokoban VERSION 0.1.0 LANGUAGES C ASM)
set(CMAKE_C_STANDARD 11)
include(GenerateG3A)
find_package(Gint 2.11 REQUIRED)
add_executable(sokoban src/main.c src/app.c src/game/game.c
  src/idle.c src/system_power.c src/system_power_syscalls.S
  src/maps/generated_maps.c src/ui/render.c src/ui/input.c
  src/storage/progress.c src/storage/codec.c src/storage/transaction.c src/storage/native.c)
target_include_directories(sokoban PRIVATE include)
target_compile_options(sokoban PRIVATE -Wall -Wextra -Werror -Wframe-larger-than=2048 -Os -g -fstack-usage)
target_link_options(sokoban PRIVATE "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/sokoban.map")
target_link_libraries(sokoban Gint::Gint)
# GenerateG3A writes into the source tree and does not create this directory.
add_custom_command(TARGET sokoban POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  VERBATIM)
generate_g3a(TARGET sokoban OUTPUT "dist/SOKOBAN.g3a" NAME "SOKOBAN"
  VERSION "00.01.0003" ICONS assets/icon-uns.png assets/icon-sel.png)
# The installed GenerateG3A has no INTERNAL argument. Assign our own namespace
# after generation; never share a default/DIFF EQ internal identity.
add_custom_command(TARGET sokoban POST_BUILD
  COMMAND fxgxa -e "${CMAKE_CURRENT_SOURCE_DIR}/dist/SOKOBAN.g3a" --internal=@SOKOBAN
  VERBATIM)
