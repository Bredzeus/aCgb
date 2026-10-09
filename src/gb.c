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

static SDL_Surface *scr_surface = NULL;
static SDL_Texture *screen = NULL;
static size_t bytes_per_px = 0;

static double fps_samples[10];
static uint32_t pos = 0;

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

  screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, GB_SCREEN_W, GB_SCREEN_H);
  if (screen == NULL) {
    printf("Failed to create screen texture\n");
    return SDL_APP_FAILURE;
  }

  scr_surface = SDL_CreateSurface(GB_SCREEN_W, GB_SCREEN_H, SDL_PIXELFORMAT_ABGR1555);
  if (scr_surface == NULL) {
    printf("Failed to create surface for screen texture\n");
    return SDL_APP_FAILURE;
  }

  bytes_per_px = SDL_BYTESPERPIXEL(scr_surface->format);

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
  clock_t time;
  time = clock();

  if(!SDL_LockTextureToSurface(screen, NULL, &scr_surface)) {
    printf("uh oh, failed to lock screen texture\n");
  }

  for (uint8_t x = 0; x < GB_SCREEN_W; x++) {
    for (uint8_t y = 0; y < GB_SCREEN_H; y++) {
      uint16_t color = ((uint16_t)x << 8) | (uint16_t)y;
      gb_draw_v2(x, y, color);
    }
  }

  SDL_UnlockTexture(screen);

  time = clock() - time;

  // Todo: render texture + present should be at the monitor refresh rate
  // not once per frame since it tanks fps
  SDL_RenderTexture(renderer, screen, NULL, NULL);
  SDL_RenderPresent(renderer);
  double t_diff = ((double)time) / CLOCKS_PER_SEC;
  double pot_fps = 1 / t_diff;
  fps_samples[pos] = pot_fps;
  pos++;
  pos %= 10;
  double fps_avg = 0;
  for (uint32_t i = 0; i < 10; i++) {
    fps_avg += fps_samples[i];
  }
  fps_avg /= 10;
  printf("delta_t total=%f, potential fps=%f\n", t_diff, pot_fps);
  printf("avg fps=%f\n", fps_avg);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  SDL_DestroyTexture(screen);
  SDL_DestroySurface(scr_surface);
  SDL_Quit();
}

void gb_draw(uint8_t x, uint8_t y, uint16_t color) {
  // reimplement part of SDL_WriteSurfacePixel
  // Todo: fix this
  uint8_t *p = (uint8_t *)scr_surface->pixels + y * scr_surface->pitch + x * bytes_per_px;
  memcpy(p, &color, bytes_per_px);
}