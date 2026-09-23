#include "gpu.h"


// LCD Control bits
#define GPU_LCDC_BG_WIN_EN_PRIO (0U)
#define GPU_LCDC_OBJ_EN (1U)
#define GPU_LCDC_OBJ_SIZE (2U)
#define GPU_LCDC_BG_TILE_MAP (3U)
#define GPU_LCDC_BG_WIN_TILE_DATA (4U)
#define GPU_LCDC_WIN_EN (5U)
#define GPU_LCDC_WIN_TILE_MAP (6U)
#define GPU_LCDC_PPU_EN (7U)

// LCD status bits
#define GPU_LCDS_PPU_MODE (0U) // 2 bits
#define GPU_LCDS_LYC (2U)
#define GPU_LCDS_MODE0_INT (3U)
#define GPU_LCDS_MODE1_INT (4U)
#define GPU_LCDS_MODE2_INT (5U)
#define GPU_LCDS_LYC_INT_SEL (6U)

static uint8_t gpu_lcd_control = 0;
static uint8_t gpu_lcd_status = 0;
static uint8_t gpu_lcd_y = 0;
static uint8_t gpu_lcd_y_compare = 0;