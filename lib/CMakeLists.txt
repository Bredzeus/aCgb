cmake_minimum_required(VERSION 3.16)

project(aCgb_lib_sdl)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

# SDL
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# SDL_mixer
if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
   AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
  # Work around because `mpg123` doesn't currently build on Linux/Arm64
  set(SDLMIXER_VENDORED OFF)
else()
  set(SDLMIXER_VENDORED ON)
endif()
set(SDLMIXER_FLAC OFF)
set(SDLMIXER_GME OFF)
set(SDLMIXER_MOD OFF)
set(SDLMIXER_MP3 OFF)
if(MINGW)
  # Prefer dr_mp3 on Windows/MinGW ARM64 to avoid libmpg123 POSIX cpu-probe code
  set(SDLMIXER_MP3_DRMP3 ON  CACHE BOOL "" FORCE)
  set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
endif()
if(EMSCRIPTEN)
  # Use dr_mp3 instead of mpg123 for Emscripten (mpg123 requires ASM)
  set(SDLMIXER_MP3_DRMP3 ON  CACHE BOOL "" FORCE)
  set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
endif()
set(SDLMIXER_MIDI OFF)
set(SDLMIXER_MIDI_NATIVE OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_WAVE OFF)
set(SDLMIXER_WAVPACK OFF)
add_subdirectory(SDL_mixer EXCLUDE_FROM_ALL)

# SDL_ttf
set(SDLTTF_VENDORED ON)
add_subdirectory(SDL_ttf EXCLUDE_FROM_ALL)

# Build all static libraries as part of the default target
add_custom_target(deps ALL DEPENDS
  SDL3::SDL3
  SDL3_mixer::SDL3_mixer
  SDL3_ttf::SDL3_ttf
)