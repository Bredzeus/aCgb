#include <stdio.h>

#include "common.h"
#include "sstr.h"
#include "sm83.h"
#include "mem.h"
#include "rom.h"


int main() {
  mem_init(rom_load, ram_store);

  sm83_t sm83;
  sm83_init(&sm83, mem_load, mem_store);
  sm83_reset(&sm83);

  //rom_load_from_file(ROM_FP);

  sstr_t debug;
  bool ok = sstr_init(&debug);
  if (ok) {
    sm83_to_sstr(&sm83, &debug);
    sstr_print(&debug);
  }
  sstr_deinit(&debug);

  return 0;
}