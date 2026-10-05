#ifndef GB_H_
#define GB_H_

#include <stdint.h>

uint8_t gb_mem_load(uint16_t addr);
void gb_mem_store(uint16_t addr, uint8_t val);

#endif // GB_H_