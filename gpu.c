#include "gpu.h"
#include "cbuf.h"

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

#define GPU_PIXEL_BUF_SIZE (16U)

typedef enum gpu_mode_s {
  GPU_MODE_HBLANK = 0U,
  GPU_MODE_VBLANK,
  GPU_MODE_OAM,
  GPU_MODE_DRAW,
  NUM_GPU_MODE
} gpu_mode_t;

typedef struct gpu_pixel_s {
  gpu_color_index_t color;
  uint8_t palette;
  uint8_t obj_priority; // Not used on DMG only CGB
  uint8_t bg_priority;
} gpu_pixel_t;

static const uint16_t GPU_MODE_MAX_DOTS[NUM_GPU_MODE] = {
  204,
  4560,
  80,
  289
};

static uint8_t gpu_lcd_control = 0;
static uint8_t gpu_lcd_status = 0;
static uint8_t gpu_lcd_y = 0;
static uint8_t gpu_lcd_y_compare = 0;

static cbuf_t gpu_bg_pixel_fifo;
static cbuf_t gpu_obj_pixel_fifo;

static gpu_mode_t gpu_state;