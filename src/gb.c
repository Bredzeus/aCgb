#include <stdio.h>
#include <time.h>

#include "common.h"
#include "sstr.h"
#include "sm83.h"
#include "mem.h"
#include "rom.h"
#include "gpu.h"
#include "input.h"
#include "timer.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define GB_WINDOW_SCALE (3U)

typedef struct input_map_s {
  SDL_Scancode button_right;
  SDL_Scancode button_left;
  SDL_Scancode button_up;
  SDL_Scancode button_down;
  SDL_Scancode button_A;
  SDL_Scancode button_B;
  SDL_Scancode button_select;
  SDL_Scancode button_start;
  SDL_Scancode speedup;
  SDL_Scancode pause;
} input_map_t;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static SDL_Surface *scr_surface = NULL;
static SDL_Texture *screen = NULL;
static size_t bytes_per_px = 0;

static sm83_t sm83;

static input_map_t keyboard_map = {
  SDL_SCANCODE_D,
  SDL_SCANCODE_A,
  SDL_SCANCODE_W,
  SDL_SCANCODE_S,
  SDL_SCANCODE_J,
  SDL_SCANCODE_K,
  SDL_SCANCODE_M,
  SDL_SCANCODE_N,
  SDL_SCANCODE_UNKNOWN,
  SDL_SCANCODE_UNKNOWN
};

static void gb_draw(uint8_t x, uint8_t y, uint16_t color);
static bool init_renderer(void);

static void gb_read_keyboard(input_state_t* keyboard_state);

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  SDL_SetAppMetadata(ACGB_NAME, ACGB_VERSION, "");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!init_renderer()) {
    return SDL_APP_FAILURE;
  }

  mem_interface_t rom_io = {&rom_load, &ram_store};
  mem_interface_t gpu_io = {&gpu_read_reg, &gpu_write_reg};
  mem_interface_t mem_io = {&mem_load, &mem_store};
  mem_interface_t input_io = {&input_load, &input_store};
  mem_interface_t timer_io = {&timer_reg_load, &timer_reg_store};

  mem_init(&rom_io, &gpu_io, &input_io, &timer_io);

  sm83_init(&sm83, &mem_io);
  sm83_reset(&sm83);

  input_enable_device(INPUT_DEVICE_KEYBOARD, true);
  timer_init(&mem_io);

  if (argc > 1) {
    if (!rom_load_from_file(argv[1])) {
      printf("Failed to load rom from file\n");
      return SDL_APP_FAILURE;
    }
  } else {
    printf("Invalid number of arguments, must provide rom filepath\n");
    return SDL_APP_FAILURE;
  }

  gpu_init(&mem_io, false, gb_draw);

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
  uint64_t start = SDL_GetTicks();

  if(!SDL_LockTextureToSurface(screen, NULL, &scr_surface)) {
    printf("Failed to lock screen texture\n");
  }
  input_state_t keyboard;
  gb_read_keyboard(&keyboard);
  input_set_device_state(INPUT_DEVICE_KEYBOARD, &keyboard);
#if ACGB_SHOW_KB == 1
  printf("%u%u%u%u %u%u%u%u\n", 
    keyboard.button_left,
    keyboard.button_right,
    keyboard.button_up,
    keyboard.button_down,
    keyboard.button_A,
    keyboard.button_B,
    keyboard.button_start,
    keyboard.button_select);
#endif // ACGB_SHOW_KB == 1

#if ACGB_DEBUG == 1

#else
  uint32_t t_cycles = 0;
  while (t_cycles < GB_CLOCKS_PER_FRAME) {
    uint32_t m_cycles = sm83_run(&sm83);
    uint32_t new_t_cycles = (4 * m_cycles);
    gpu_run(new_t_cycles);
    mem_run_dma_controller(m_cycles);
    timer_run(m_cycles);
    t_cycles += new_t_cycles;
  }

#endif // ACGB_DEBUG == 1

  SDL_UnlockTexture(screen);

  // Todo: render texture + present should be at the monitor refresh rate
  // not once per frame since it tanks fps
  SDL_RenderTexture(renderer, screen, NULL, NULL);
  SDL_RenderPresent(renderer);

  uint64_t diff = SDL_GetTicks() - start;
  if (diff < TICKS_PER_FRAME) {
    SDL_Delay(TICKS_PER_FRAME - diff);
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  gpu_deinit();
  rom_cleanup();
  SDL_DestroyTexture(screen);
  SDL_DestroySurface(scr_surface);
  SDL_Quit();
}

void gb_draw(uint8_t x, uint8_t y, uint16_t color) {
  // reimplements part of SDL_WriteSurfacePixel
  uint16_t color_alpha_fix = color | 0x8000;
  uint8_t *p = (uint8_t *)scr_surface->pixels + y * scr_surface->pitch + x * bytes_per_px;
  memcpy(p, &color_alpha_fix, bytes_per_px);
}

bool init_renderer(void) {
  if (!SDL_CreateWindowAndRenderer(ACGB_NAME, GPU_SCREEN_WIDTH * GB_WINDOW_SCALE, GPU_SCREEN_HEIGHT * GB_WINDOW_SCALE, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return false;
  }
  SDL_SetRenderLogicalPresentation(renderer, GPU_SCREEN_WIDTH, GPU_SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, GPU_SCREEN_WIDTH, GPU_SCREEN_HEIGHT);
  if (screen == NULL) {
    printf("Failed to create screen texture\n");
    return false;
  }

  scr_surface = SDL_CreateSurface(GPU_SCREEN_WIDTH, GPU_SCREEN_HEIGHT, SDL_PIXELFORMAT_ABGR1555);
  if (scr_surface == NULL) {
    printf("Failed to create surface for screen texture\n");
    return false;
  }

  bytes_per_px = SDL_BYTESPERPIXEL(scr_surface->format);

  printf("Window size: %ux%u (x%u)\n", GPU_SCREEN_WIDTH * GB_WINDOW_SCALE, GPU_SCREEN_HEIGHT * GB_WINDOW_SCALE, GB_WINDOW_SCALE);
  printf("Window logical size: %ux%u\n", GPU_SCREEN_WIDTH, GPU_SCREEN_HEIGHT);
  printf("Bytes per pixel: %lu\n", bytes_per_px);

  return true;
}

void gb_read_keyboard(input_state_t* keyboard_state) {
  const bool* key_states = SDL_GetKeyboardState(NULL);
  keyboard_state->button_right = key_states[keyboard_map.button_right];
  keyboard_state->button_left = key_states[keyboard_map.button_left];
  keyboard_state->button_up = key_states[keyboard_map.button_up];
  keyboard_state->button_down = key_states[keyboard_map.button_down];
  keyboard_state->button_A = key_states[keyboard_map.button_A];
  keyboard_state->button_B = key_states[keyboard_map.button_B];
  keyboard_state->button_select = key_states[keyboard_map.button_select];
  keyboard_state->button_start = key_states[keyboard_map.button_start];
  keyboard_state->speedup = key_states[keyboard_map.speedup];
  keyboard_state->pause = key_states[keyboard_map.pause];
}