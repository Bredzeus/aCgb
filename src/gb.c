#include <stdio.h>
#include <time.h>

#include "common.h"
#include "sstr.h"
#include "sm83.h"
#include "mem.h"
#include "rom.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define GB_SCREEN_H (144U)
#define GB_SCREEN_W (160U)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static uint64_t last_time = 0;

static void gb_draw(uint8_t x, uint8_t y, uint16_t color);

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  SDL_SetAppMetadata("Test", "0.1", "");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("idk", GB_SCREEN_W, GB_SCREEN_H, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderLogicalPresentation(renderer, GB_SCREEN_W, GB_SCREEN_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  mem_init(rom_load, ram_store);

  mem_interface_t mem_io = {
    &mem_load, 
    &mem_store
  };

  sm83_t sm83;
  sm83_init(&sm83, &mem_io);
  sm83_reset(&sm83);

  ////rom_load_from_file(ROM_FP);

  sstr_t debug;
  bool ok = sstr_init(&debug);
  if (ok) {
    sm83_to_sstr(&sm83, &debug);
    sstr_print(&debug);
  }
  sstr_deinit(&debug);

  last_time = SDL_GetTicks();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  // event callback
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  // once per frame update
  //SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  //SDL_RenderClear(renderer);
  clock_t time;
  time = clock();
  for (uint8_t x = 0; x < GB_SCREEN_W / 2; x++) {
    for (uint8_t y = 0; y < GB_SCREEN_H / 2; y++) {
      uint16_t color = ((uint16_t)x << 8) | (uint16_t)y;
      gb_draw(x, y, color);
    }
  }

  SDL_RenderPresent(renderer);


  time = clock() - time;
  double t_diff = ((double)time) / CLOCKS_PER_SEC;
  double pot_fps = 1 / t_diff;
  printf("delta_t=%f, potential fps=%f\n", t_diff, pot_fps);
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  SDL_Quit();
}


void gb_draw(uint8_t x, uint8_t y, uint16_t color) {
  // way too slow
  // fps capped at ~30 drawing 160 by 144
  uint8_t r = (color & 0x001F) << 3;
  uint8_t g = (color & 0x03E0) >> 2;
  uint8_t b = (color & 0x7C00) >> 7;
  SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
  SDL_RenderPoint(renderer, (float)x, (float)y);
}