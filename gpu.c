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
  // might need to combine these priorities
  uint8_t obj_priority; // Not used on DMG only CGB
  uint8_t bg_priority;
} gpu_pixel_t;

static const uint16_t GPU_MODE_MAX_DOTS[NUM_GPU_MODE] = {
  204,
  4560,
  80,
  289
};

// Local variables
static uint8_t gpu_lcd_control = 0;
static uint8_t gpu_lcd_status = 0;
static uint8_t gpu_lcd_y = 0;
static uint8_t gpu_lcd_y_compare = 0;

static cbuf_t gpu_bg_pixel_fifo;
static cbuf_t gpu_obj_pixel_fifo;

static gpu_mode_t gpu_state = GPU_MODE_OAM;
static bool gpu_is_cgb = false;
static uint8_t gpu_px_x = 0;
static uint8_t gpu_px_y = 0;

static draw_f_t gpu_draw = NULL;

// Local functions

static inline uint16_t gpu_make_color(uint8_t palette_h, uint8_t palette_l);
static void gpu_render(void);
static uint16_t gpu_get_cgb_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px);
static uint16_t gpu_get_dmg_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px);





uint16_t gpu_make_color(uint8_t palette_h, uint8_t palette_l) {
  return (((uint16_t)palette_h << 8) | (uint16_t)palette_l);
}

void gpu_render(void) {
  gpu_pixel_t* bg_px = NULL;
  gpu_pixel_t* obj_px = NULL;
  bool priority_obj = false;
  uint16_t color = 0;

  // Check pixel buffers
  if (cbuf_size(&gpu_bg_pixel_fifo) != 0) {
    bg_px = (gpu_pixel_t*)cbuf_rd(&gpu_bg_pixel_fifo);
  }
  if (cbuf_size(&gpu_obj_pixel_fifo) != 0) {
    obj_px = (gpu_pixel_t*)cbuf_rd(&gpu_obj_pixel_fifo);
  }
  if (bg_px == NULL || gpu_px_x >= GPU_SCREEN_WIDTH) {
    return;
  }

  // Color logic based on hardware
  if (gpu_is_cgb) {
    color = gpu_get_cgb_color(bg_px, obj_px);
  } else {
    color = gpu_get_dmg_color(bg_px, obj_px);
  }
  gpu_draw(gpu_px_x, gpu_px_y, color);
}

uint16_t gpu_get_cgb_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px) {

}

uint16_t gpu_get_dmg_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px) {
  uint16_t color = 0;
  bool obj_priority = false;
  if (obj_px != NULL) {
    if (obj_px->color != CPU_COLOR_INDEX_0 
      && GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_EN)) {
      obj_priority = (obj_px->priority >= bg_px->priority);
    }
  }
  if (obj_priority) {
    color = obj_px->palette;
  } else {
    if (GET_BIT(gpu_lcd_control, GPU_LCDC_BG_WIN_EN_PRIO)) {
      // color based on bg_px
      color = bg_px->palette;
    } else {
      color = 0; // bg is disabled
    }
  }
  return color;
}