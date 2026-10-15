#ifndef GPU_H_
#define GPU_H_

#include "common.h"
#include "sstr.h"

#include <stdbool.h>

#define GPU_SCREEN_WIDTH (160U)
#define GPU_SCREEN_HEIGHT (144U)

#define ADDR_VRAM_TILE_END (0x97FFU)
#define ADDR_VRAM_BLOCK0_START (0x8000U)
#define ADDR_VRAM_BLOCK0_END (0x87FFU)
#define ADDR_VRAM_BLOCK1_START (0x8800U)
#define ADDR_VRAM_BLOCK1_END (0x88FFU)
#define ADDR_VRAM_BLOCK2_START (0x9000U)
#define ADDR_VRAM_BLOCK2_END (0x97FFU)
#define ADDR_VRAM_TILEMAP0_START (0x9800U)
#define ADDR_VRAM_TILEMAP0_END (0x9BFFU)
#define ADDR_VRAM_TILEMAP1_START (0x9C00U)
#define ADDR_VRAM_TILEMAP1_END (0x9FFFU)

bool gpu_init(mem_interface_t* mem_interface, bool is_cgb);
void gpu_deinit(void);

uint8_t gpu_read_reg(uint16_t addr);
void gpu_write_reg(uint16_t addr, uint8_t val);
void gpu_run(uint8_t clocks);

#endif // GPU_H_