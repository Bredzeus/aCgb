#include <stdio.h>
#include <stdlib.h>
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
#define GB_DEBUG_ARG ("--debug")

typedef enum gb_state_s {
  GB_STATE_PAUSE,
  GB_STATE_RUN,
  NUM_GB_STATES
} gb_state_t;

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

typedef enum debug_command_s {
  DEBUG_COMMAND_RAM = 0,
  DEBUG_COMMAND_SM83,
  DEBUG_COMMAND_RUN,
  DEBUG_COMMAND_EXIT,
  DEBUG_COMMAND_BREAK,
  NUM_DEBUG_COMMANDS
} debug_command_t;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static SDL_Surface *scr_surface = NULL;
static SDL_Texture *screen = NULL;
static size_t bytes_per_px = 0;

static sm83_t gb_sm83;
static gb_state_t gb_state;

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
  SDL_SCANCODE_P
};

static SDL_AppResult app_result = SDL_APP_CONTINUE;
static bool breakpoint_en = false;
static bool breakpoint_reset = false;
static uint16_t breakpoint_addr = 0x0000U;
static uint64_t gb_instr_count = 0U;

static void gb_draw(uint8_t x, uint8_t y, uint16_t color);
static bool init_renderer(void);

static void gb_read_keyboard(input_state_t* keyboard_state);
static void gb_handle_debug_commands(void);
static void gb_run(uint32_t t_cycles);

static void gb_debug_dump_memory(uint16_t addr, uint32_t num_bytes);
static void gb_debug_run(uint32_t num_instr);
static void gb_dump_sm83(void);
static void gb_breakpoint(bool en, uint16_t addr);

static void gb_reset_sm83_dmg(sm83_t* sm83, uint8_t checksum);
static void gb_reset_sm83_cgb(sm83_t* sm83);

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

  sm83_init(&gb_sm83, &mem_io);

  input_enable_device(INPUT_DEVICE_KEYBOARD, true);
  timer_init(&mem_io);

  bool rom_arg = false;
  gb_state = GB_STATE_RUN;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], GB_DEBUG_ARG) == 0) {
      gb_state = GB_STATE_PAUSE;
    } else {
      rom_arg = true;
      if (!rom_load_from_file(argv[i])) {
        printf("Failed to load rom from file\n");
        return SDL_APP_FAILURE;
      }
    }
  }
  if (!rom_arg) {
    printf("Invalid number of arguments, must provide rom filepath\n");
    return SDL_APP_FAILURE;
  }

  gpu_init(&mem_io, false, gb_draw);

  rom_info_t rom_info;
  rom_get_info(&rom_info);
  if (GET_BIT(rom_info.cgb_flag, 7U)) {
    gb_reset_sm83_cgb(&gb_sm83);
  } else {
    gb_reset_sm83_dmg(&gb_sm83, rom_info.header_checksum);
  }

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

  if (gb_state == GB_STATE_PAUSE) {
    gb_handle_debug_commands();
  } else if (gb_state == GB_STATE_RUN) {
    gb_run(GB_CLOCKS_PER_FRAME);
  }

  uint64_t diff = SDL_GetTicks() - start;
  if (diff < TICKS_PER_FRAME) {
    SDL_Delay(TICKS_PER_FRAME - diff);
  }

  return app_result;
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
#if ACGB_SHOW_KB == 1
  printf("%u%u%u%u %u%u%u%u %u%u\n", 
    keyboard_state->button_left,
    keyboard_state->button_right,
    keyboard_state->button_up,
    keyboard_state->button_down,
    keyboard_state->button_A,
    keyboard_state->button_B,
    keyboard_state->button_start,
    keyboard_state->button_select,
    keyboard_state->speedup,
    keyboard_state->pause);
#endif // ACGB_SHOW_KB == 1
}

void gb_handle_debug_commands(void) {
  #define DEBUG_INPUT_SIZE (100U)
  char input_buffer[DEBUG_INPUT_SIZE];
  printf("Instruction count: %lu\n", gb_instr_count);
  printf("Enter command: ");
  fgets(input_buffer, sizeof(input_buffer), stdin);
  debug_command_t debug_command = NUM_DEBUG_COMMANDS;
  uint32_t arg_idx = 0;
  uint32_t args[2];
  char* token = strtok(input_buffer, " ");
  while (token != NULL && arg_idx < 3U) {
    if (arg_idx == 0) {
      if (strncmp(token, "ram", 3U) == 0) {
        debug_command = DEBUG_COMMAND_RAM;
      } else if (strncmp(token, "sm83", 4U) == 0) {
        debug_command = DEBUG_COMMAND_SM83;
      } else if (strncmp(token, "run", 3U) == 0) {
        debug_command = DEBUG_COMMAND_RUN;
      } else if (strncmp(token, "exit", 4U) == 0) {
        debug_command = DEBUG_COMMAND_EXIT;
      } else if (strncmp(token, "break", 5U) == 0) {
        debug_command = DEBUG_COMMAND_BREAK;
      }
    } else {
      args[arg_idx - 1] = strtoul(token, NULL, 10);
    }
    arg_idx++;
    token = strtok(NULL, " ");
  }
  switch (debug_command) {
    case DEBUG_COMMAND_RAM:
      gb_debug_dump_memory(args[0], args[1]);
      break;
    case DEBUG_COMMAND_SM83:
      gb_dump_sm83();
      break;
    case DEBUG_COMMAND_RUN:
      gb_debug_run(args[0]);
      break;
    case DEBUG_COMMAND_EXIT:
      app_result = SDL_APP_SUCCESS;
      break;
    case DEBUG_COMMAND_BREAK:
      gb_breakpoint(args[0], args[1]);
      break;
    default:
      break;
  }
}

void gb_run(uint32_t t_cycles) {
  if(!SDL_LockTextureToSurface(screen, NULL, &scr_surface)) {
    printf("Failed to lock screen texture\n");
  }
  input_state_t keyboard;
  gb_read_keyboard(&keyboard);
  input_set_device_state(INPUT_DEVICE_KEYBOARD, &keyboard);
  if (keyboard.pause) {
    printf("pausing and entering debug mode\n");
    gb_state = GB_STATE_PAUSE;
  }
  uint32_t cycle = 0U;
  while (cycle < t_cycles) {
    if (breakpoint_en && !breakpoint_reset && gb_sm83.reg_file.pc == breakpoint_addr) {
      printf("breakpoint hit\n");
      gb_state = GB_STATE_PAUSE;
      breakpoint_reset = true;
      break;
    }
    breakpoint_reset = false;
    uint32_t m_cycles = sm83_run(&gb_sm83);
    gb_instr_count++;
    uint32_t new_t_cycles = (4 * m_cycles);
    gpu_run(new_t_cycles);
    mem_run_dma_controller(m_cycles);
    timer_run(m_cycles);
    cycle += new_t_cycles;
  }
  SDL_UnlockTexture(screen);

  // Todo: render texture + present should be at the monitor refresh rate
  // not once per frame since it tanks fps
  SDL_RenderTexture(renderer, screen, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void gb_debug_dump_memory(uint16_t addr, uint32_t num_bytes) {
  const uint32_t bytes_per_line = 8U;
  for (uint32_t i = 0; i < num_bytes; i++) {
    if (i % bytes_per_line == 0U) {
      printf("\n0x%04x:", addr + i);
    }
    printf(" %02x", mem_load(addr + i));
  }
  printf("\n");
}

void gb_debug_run(uint32_t num_instr) {
  if (num_instr == 0) {
    // Todo: unpausing breaks keyboard input
    // probably cause input handling is done directly rather than through sdl
    gb_state = GB_STATE_RUN;
    printf("unpausing and exiting debug mode\n");
  } else {
    gb_run(num_instr);
  }
}

void gb_dump_sm83(void) {
  sstr_t sm83_dump;
  bool ok = sstr_init(&sm83_dump);
  if (ok) {
    sm83_to_sstr(&gb_sm83, &sm83_dump);
    sstr_print(&sm83_dump);
  }
  sstr_deinit(&sm83_dump);
}

void gb_breakpoint(bool en, uint16_t addr) {
  breakpoint_en = en;
  breakpoint_addr = addr;
  breakpoint_reset = false;
}

void gb_reset_sm83_dmg(sm83_t* sm83, uint8_t checksum) {
  sm83->reg_file.pc = 0x0100;
  sm83->reg_file.sp = 0xfffe;
  sm83->reg_file.a = 0x01;
  if (checksum == 0x00U) {
    sm83->reg_file.f = 0x80;
  } else {
    sm83->reg_file.f = 0xb0;
  }
  sm83->reg_file.b = 0x00;
  sm83->reg_file.c = 0x13;
  sm83->reg_file.d = 0x00;
  sm83->reg_file.e = 0xd8;
  sm83->reg_file.h = 0x01;
  sm83->reg_file.l = 0x4d;
  sm83->reg_file.ir = 0;
  sm83->reg_file.ie = 0;

  sm83->state = SM83_STATE_RUN;
  sm83->ime = false;
}

void gb_reset_sm83_cgb(sm83_t* sm83) {
  sm83->reg_file.pc = 0x0100;
  sm83->reg_file.sp = 0xfffe;
  sm83->reg_file.a = 0x11;
  sm83->reg_file.f = 0x80;
  sm83->reg_file.b = 0x00;
  sm83->reg_file.c = 0x00;
  sm83->reg_file.d = 0xff;
  sm83->reg_file.e = 0x56;
  sm83->reg_file.h = 0x00;
  sm83->reg_file.l = 0x0d;
  sm83->reg_file.ir = 0;
  sm83->reg_file.ie = 0;

  sm83->state = SM83_STATE_RUN;
  sm83->ime = false;
}