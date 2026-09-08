#ifndef MEM_H_
#define MEM_H_

#include "common.h"

void mem_init(mem_load_f_t rom_load, mem_store_f_t rom_store);
uint8_t mem_load(uint16_t addr);
void mem_store(uint16_t addr, uint8_t val);

#endif