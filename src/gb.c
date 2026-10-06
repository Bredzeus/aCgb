#include <stdio.h>

#include "common.h"
#include "sstr.h"
#include "sm83.h"
#include "mem.h"
#include "rom.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static uint64_t last_time = 0;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  SDL_SetAppMetadata("Test", "0.1", "");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("idk", 160, 144, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_LETTERBOX);

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

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}
