#ifndef MEM_H_
#define MEM_H_

#include "common.h"

void mem_init(mem_interface_t *rom_io, mem_interface_t *gpu_io);
uint8_t mem_load(uint16_t addr);
void mem_store(uint16_t addr, uint8_t val);

#endif