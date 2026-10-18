#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include "common.h"

void timer_init(mem_interface_t* mem_io);
uint8_t timer_reg_load(uint16_t addr);
void timer_reg_store(uint16_t addr, uint8_t val);
void timer_run(uint8_t m_cycles);

#endif // TIMER_H_