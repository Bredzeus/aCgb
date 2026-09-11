#ifndef COMMON_H_
#define COMMON_H_

#include "mem_map.h"

#include <stdint.h>
#include <stddef.h>

#define GB_MAIN_CLOCK_HZ (4194304U)
#define GBC_MAIN_CLOCK_HZ (8388608U)

#define INT_VBLANK (0x1U)
#define INT_LCD (0x1U << 1)
#define INT_TIMER (0x1U << 2)
#define INT_SERIAL (0x1U << 3)
#define INT_JOYPAD (0x1U << 4)

typedef uint8_t (*mem_load_f_t)(uint16_t);
typedef void (*mem_store_f_t)(uint16_t, uint8_t);
typedef uint8_t (*rom_load_f_t)(size_t);
typedef void (*rom_store_f_t)(size_t, uint8_t);

inline bool check_range_inc(unsigned val, unsigned lower, unsigned upper) {
  return (val >= lower && val <= upper);
}

inline bool check_range_exc(unsigned val, unsigned lower, unsigned upper) {
  return (val > lower && val < upper);
}

#endif 