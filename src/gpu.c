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

#define GPU_NUM_CGB_PALETTES (8U)
#define GPU_BYTES_PER_PALETTE (8U)
#define GPU_BYTES_PER_COLOR (2U)

#define GPU_TILES_PER_MAP_DIM (32U)

#define GPU_DMG_BG_COLOR_IDX_WHITE (GPU_COLOR_INDEX_0)
#define GPU_DMG_BG_COLOR_IDX_LIGHT_GRAY (GPU_COLOR_INDEX_1)
#define GPU_DMG_BG_COLOR_IDX_DARK_GRAY (GPU_COLOR_INDEX_2)
#define GPU_DMG_BG_COLOR_IDX_BLACK (GPU_COLOR_INDEX_3)

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

// Indexed by gpu colors
const uint16_t gpu_dmg_color_values[NUM_GPU_COLOR_INDEX] = {
  0xFFFFFFU, // white
  0xB5B5B5U, // light gray
  0x545454U, // dark gray
  0x000000U // black
}

// Local variables
static uint8_t gpu_lcd_control = 0; // 0xFF40
static uint8_t gpu_lcd_status = 0; // 0xFF41
static uint8_t gpu_view_y = 0; // 0xFF42
static uint8_t gpu_view_x = 0; // 0xFF43
static uint8_t gpu_lcd_y = 0; // 0xFF44
static uint8_t gpu_lcd_y_compare = 0; // 0xFF45
static uint8_t gpu_oam_dma = 0; // 0xFF46
static uint8_t gpu_dmg_bg_palette = 0; // 0xFF47
static uint8_t gpu_dmg_obj_palette_0 = 0; // 0xFF48
static uint8_t gpu_dmg_obj_palette_1 = 0; // 0xFF49
static uint8_t gpu_window_y = 0; // 0xFF4A
static uint8_t gpu_window_x = 0; // 0xFF4B

static uint8_t gpu_bg_palette_idx = 0; // 0xFF68
static uint8_t gpu_bg_palette_data = 0; // 0xFF69
static uint8_t gpu_obj_palette_idx = 0; // 0xFF6A
static uint8_t gpu_obj_palette_data = 0; // 0xFF6B

static uint8_t gpu_cgb_bg_palettes[GPU_NUM_CGB_PALETTES * GPU_BYTES_PER_PALETTE];
static uint8_t gpu_cgb_obj_palettes[GPU_NUM_CGB_PALETTES * GPU_BYTES_PER_PALETTE];

static cbuf_t gpu_bg_pixel_fifo;
static cbuf_t gpu_obj_pixel_fifo;

static gpu_mode_t gpu_state = GPU_MODE_OAM;
static bool gpu_is_cgb = false;
static uint8_t gpu_px_x = 0;
static uint8_t gpu_px_y = 0;
static uint8_t gpu_fetcher_px_x = 0;
static uint8_t gpu_fetcher_px_y = 0;
static bool gpu_window_y_cond = false;

static uint16_t (*gpu_get_color)(gpu_pixel_t*, gpu_pixel_t*);
static draw_f_t gpu_draw = NULL;
static mem_interface_t gpu_io;

// Local function declarations

static uint8_t* gpu_reg_lookup(uint16_t addr);
static inline uint16_t gpu_make_color(uint8_t color_h, uint8_t color_l);
static void gpu_render(void);
static uint16_t gpu_get_cgb_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px);
static uint16_t gpu_get_dmg_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px);
static void gpu_fetch_pixels(void);
static uint16_t gpu_get_tilemap_addr(uint16_t window_x_start, uint16_t window_x_end);
static uint8_t gpu_fetcher_get_x_coord(uint16_t window_x_start);
static uint8_t gpu_fetcher_get_y_coord(uint16_t window_x_start);

// Interface function implementations

bool gpu_init(mem_interface_t* mem_interface, bool is_cgb) {
  gpu_io.mem_load = mem_interface->mem_load;
  gpu_io.mem_store = mem_interface->mem_store;
  gpu_is_cgb = is_cgb;
  // Color logic based on hardware
  if (is_cgb) {
    gpu_get_color = gpu_get_cgb_color;
    memset(&gpu_cgb_bg_palettes, 0xFF, sizeof(gpu_cgb_bg_palettes) / sizeof(uint8_t));
  } else {
    gpu_get_color = gpu_get_dmg_color;
  }
  bool ok = cbuf_init(&gpu_bg_pixel_fifo, sizeof(gpu_pixel_t), GPU_PIXEL_BUF_SIZE);
  ok &= cbuf_init(&gpu_obj_pixel_fifo, sizeof(gpu_pixel_t), GPU_PIXEL_BUF_SIZE);
  return ok;
}

void gpu_deinit(void) {
  cbuf_deinit(&gpu_bg_pixel_fifo);
  cbuf_deinit(&gpu_obj_pixel_fifo);
}

uint8_t gpu_read_reg(uint16_t addr) {
  uint8_t val = 0;
  uint8_t* reg_ptr = gpu_reg_lookup(addr);
  if (reg_ptr != NULL) {
    val = *reg_ptr;
  }
  return val;
}

void gpu_write_reg(uint16_t addr, uint8_t val) {
  uint8_t* reg_ptr = gpu_reg_lookup(addr);
  if (reg_ptr != NULL) {
    *reg_ptr = val;
  }
}


// Local function implementations

uint8_t* gpu_reg_lookup(uint16_t addr) {
  uint8_t reg = NULL;
  switch (addr) {
    case 0xFF40:
      reg = &gpu_lcd_control;
      break;
    case 0xFF41:
      reg = &gpu_lcd_status;
      break;
    case 0xFF42:
      reg = &gpu_view_y;
      break;
    case 0xFF43:
      reg = &gpu_view_x;
      break;
    case 0xFF44:
      reg = &gpu_lcd_y;
      break;
    case 0xFF45:
      reg = &gpu_lcd_y_compare;
      break;
    case 0xFF46:
      reg = &gpu_oam_dma;
      break;
    case 0xFF47:
      reg = &gpu_dmg_bg_palette;
      break;
    case 0xFF48:
      reg = &gpu_dmg_obj_palette_0;
      break;
    case 0xFF49:
      reg = &gpu_dmg_obj_palette_1;
      break;
    case 0xFF4A:
      reg = &gpu_window_y;
      break;
    case 0xFF4B:
      reg = &gpu_window_x;
      break;
    case 0xFF68:
      reg = &gpu_bg_palette_idx;
      break;
    case 0xFF69:
      reg = &gpu_bg_palette_data;
      break;
    case 0xFF6A:
      reg = &gpu_obj_palette_idx;
      break;
    case 0xFF6B:
      reg = &gpu_obj_palette_data;
      break;
    default:
      reg = NULL;
      break;
  }
  return reg;
}

uint16_t gpu_make_color(uint8_t color_h, uint8_t color_l) {
  return (((uint16_t)color_h << 8) | (uint16_t)color_l);
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
  color = gpu_get_color(bg_px, obj_px);
  gpu_draw(gpu_px_x, gpu_px_y, color);
}

uint16_t gpu_get_cgb_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px) {
  uint16_t color = 0;
  bool obj_priority = false;
    if (!GET_BIT(gpu_lcd_control, GPU_LCDC_BG_WIN_EN_PRIO)) {
    obj_priority = true;
  }
  // Does the obj pixel have priority?
  if (obj_px != NULL) {
    if (obj_px->color != CPU_COLOR_INDEX_0 
      && GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_EN)) {
      obj_priority = (obj_px->bg_priority >= bg_px->bg_priority);
    }
  } else if (obj_priority) {
    // Obj was forced to have priority by LCDC.0 but there is no pixel
    // return black pixel
    return gpu_dmg_color_values[GPU_DMG_BG_COLOR_IDX_BLACK];
  }
  if (obj_priority) {
    // color based on object pixel
    uint8_t color_idx = (obj_px->palette * GPU_BYTES_PER_PALETTE) + (obj_px->color * GPU_BYTES_PER_COLOR);
    uint8_t color_low = gpu_cgb_obj_palettes[color_idx];
    uint8_t color_high = gpu_cgb_obj_palettes[color_idx + 1];
    color = gpu_make_color(color_high, color_low);
  } else {
    // color based on background pixel
    uint8_t color_idx = (bg_px->palette * GPU_BYTES_PER_PALETTE) + (bg_px->color * GPU_BYTES_PER_COLOR);
    uint8_t color_low = gpu_cgb_bg_palettes[color_idx];
    uint8_t color_high = gpu_cgb_bg_palettes[color_idx + 1];
    color = gpu_make_color(color_high, color_low);
  }
  return color;
}

uint16_t gpu_get_dmg_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px) {
  uint16_t color = 0;
  bool obj_priority = false;
  // Does the obj pixel have priority?
  if (obj_px != NULL) {
    if (obj_px->color != CPU_COLOR_INDEX_0 
      && GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_EN)) {
      obj_priority = (obj_px->bg_priority >= bg_px->bg_priority);
    }
  }
  if (obj_priority) {
    // color based on object pixel
    uint8_t color_val_idx = 0;
    if (obj_px->palette != 0) {
      color_val_idx = (gpu_obj_palette_1 >> (obj_px->color * 2)) & 0x03U;
    } else {
      color_val_idx = (gpu_obj_palette_0 >> (obj_px->color * 2)) & 0x03U;
    }
    color = gpu_dmg_color_values[color_val_idx];
  } else {
    if (GET_BIT(gpu_lcd_control, GPU_LCDC_BG_WIN_EN_PRIO)) {
      // color based on background pixel
      uint8_t color_val_idx = (gpu_bg_palette >> (bg_px->color * 2)) & 0x03U;
      color = gpu_dmg_color_values[color_val_idx];
    } else {
      // background is disabled
      color = gpu_dmg_color_values[GPU_DMG_BG_COLOR_IDX_BLACK];
    }
  }
  return color;
}

void gpu_fetch_pixels(void) {
  uint16_t window_x_start = gpu_window_x - 7; // window x offset by 7 for some reason
  uint16_t window_x_end = gpu_window_x + GPU_SCREEN_WIDTH - 7;
  uint16_t window_y_start = gpu_window_y;
  uint16_t window_y_end = gpu_window_y + GPU_SCREEN_HEIGHT;
  uint16_t tilemap_addr = gpu_get_tilemap_addr(window_x_start, window_x_end);
  uint8_t x_coord = gpu_fetcher_get_x_coord(window_x_start);
  uint8_t y_coord = gpu_fetcher_get_y_coord(window_x_start);
  uint8_t tile_addr = 0xFF;
  if (gpu_state != GPU_MODE_DRAW) {
    uint16_t tile_addr = tilemap_addr + (coord_y * GPU_TILES_PER_MAP_DIM) + coord_x;
  }
  uint8_t tile_idx = gpu_io.mem_load(tile_addr);
  

}

uint16_t gpu_get_tilemap_addr(uint16_t window_x_start, uint16_t window_x_end) {
  uint16_t tilemap_addr = ADDR_VRAM_TILEMAP0_START;
  if (GET_BIT(gpu_lcd_control, GPU_LCDC_BG_TILE_MAP) 
  && !RANGE_INC(gpu_px_x, window_x_start, window_x_end)) {
    tilemap_addr = ADDR_VRAM_TILEMAP1_START;
  }
  if (GET_BIT(gpu_lcd_control, GPU_LCDC_WIN_TILE_MAP)
  && RANGE_INC(gpu_px_x, window_x_start, window_x_end)) {
    tilemap_addr = ADDR_VRAM_TILEMAP1_START;
  }
  return tilemap_addr;
}

uint8_t gpu_fetcher_get_x_coord(uint16_t window_x_start) {
  uint8_t x_coord = 0;
  if (gpu_window_y_cond && (gpu_px_x >= window_x_start)) {
    x_coord = gpu_px_x - window_x_start;
  } else {
    x_coord = (gpu_view_x / 8 + gpu_fetcher_px_x) & 0x1F;
  }
  return x_coord;
}

uint8_t gpu_fetcher_get_y_coord(uint16_t window_x_start) {
  uint8_t y_coord = 0;
  if (gpu_window_y_cond && (gpu_px_x >= window_x_start)) {
    y_coord = gpu_px_y - window_y_start;
  } else {
    y_coord = (gpu_px_y + gpu_view_y) & 0xFF;
  }
  return y_coord;
}