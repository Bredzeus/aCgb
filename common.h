#ifndef COMMON_H_
#define COMMON_H_

#include "mem_map.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GB_MAIN_CLOCK_HZ (4194304U)
#define GBC_MAIN_CLOCK_HZ (8388608U)

#define INT_VBLANK (0U)
#define INT_LCD (1U)
#define INT_TIMER (2U)
#define INT_SERIAL (3U)
#define INT_JOYPAD (4U)

#define SET_BIT(reg, bit) ((reg) |= (0x1 << (bit)))
#define CLEAR_BIT(reg, bit) ((reg) &= ~(0x1 << (bit)))
#define GET_BIT(reg, bit) (((reg) >> (bit)) & 0x1)

//#define SET_BYTE(reg, byte, val) ((reg) = ((val) << (byte)))
//#define GET_BYTE(reg, byte) (((reg) >> (byte)) & 0xFF)

#define RANGE_INC(val, lower, upper) (((val) >= (lower)) && ((val) <= (upper)))
#define RANGE_EXC(val, lower, upper) (((val) > (lower)) && ((val) < (upper)))

typedef uint8_t (*mem_load_f_t)(uint16_t);
typedef void (*mem_store_f_t)(uint16_t, uint8_t);
typedef uint8_t (*rom_load_f_t)(size_t);
typedef void (*rom_store_f_t)(size_t, uint8_t);
typedef void (*draw_f_t)(uint8_t, uint8_t, uint16_t);

typedef struct mem_interface_s{
  mem_load_f_t mem_load;
  mem_store_f_t mem_store;
} mem_interface_t;

#endif 