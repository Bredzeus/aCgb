#include <stdio.h>

#include <simple2d.h>

#include "common.h"
#include "sstr.h"
#include "sm83.h"
#include "mem.h"
#include "rom.h"

S2D_Window* window;

void S2D_Init() {
  window = S2D_CreateWindow("Test", 160, 144);

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
}

void S2D_Quit() {
  // Do not free the window here, its done automatically
  return;
}