#include "gpu.h"
#include "cbuf.h"
#include "mem_map.h"

#include <string.h>

#define GPU_DOT_WIDTH (455U)
#define GPU_DOT_HEIGHT (153U)

#define GPU_DOT_VBLANK_Y (143U)
#define GPU_DOT_DRAW_X_START (80U)
#define GPU_DOT_HBLANK_START (252U)

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
#define GPU_BYTES_PER_TILE (16U)
#define GPU_BYTES_PER_TILE_SLICE (2U)
#define GPU_BYTES_PER_OBJ (4U)
#define GPU_TILES_PER_MAP_DIM (32U)
#define GPU_NUM_OBJ_PER_SCANLINE (10U)

#define GPU_DMG_BG_COLOR_IDX_WHITE (GPU_COLOR_INDEX_0)
#define GPU_DMG_BG_COLOR_IDX_LIGHT_GRAY (GPU_COLOR_INDEX_1)
#define GPU_DMG_BG_COLOR_IDX_DARK_GRAY (GPU_COLOR_INDEX_2)
#define GPU_DMG_BG_COLOR_IDX_BLACK (GPU_COLOR_INDEX_3)

#define GPU_OAM_SCANLINE_NONE (-1)
#define GPU_OBJ_DOT_WIDTH (8U)
#define GPU_OBJ_DOT_HEIGHT_0 (8U)
#define GPU_OBJ_DOT_HEIGHT_1 (16U)
#define GPU_OBJ_Y_OFFSET (16U)
#define GPU_OBJ_X_OFFSET (8U)

typedef enum gpu_mode_s {
  GPU_MODE_HBLANK = 0U,
  GPU_MODE_VBLANK,
  GPU_MODE_OAM,
  GPU_MODE_DRAW,
  NUM_GPU_MODE
} gpu_mode_t;

typedef enum gpu_color_index_s {
  GPU_COLOR_INDEX_0 = 0, // transparent for objs
  GPU_COLOR_INDEX_1,
  GPU_COLOR_INDEX_2,
  GPU_COLOR_INDEX_3,
  NUM_GPU_COLOR_INDEX
} gpu_color_index_t;

typedef struct gpu_pixel_s {
  gpu_color_index_t color;
  uint8_t palette;
  uint8_t bg_priority;
} gpu_pixel_t;

// Indexed by gpu colors
const uint16_t gpu_dmg_color_values[NUM_GPU_COLOR_INDEX] = {
  0xFFFFU, // white
  0xDAD6U, // light gray
  0xA94AU, // dark gray
  0x8000U // black
};

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

static bool gpu_is_cgb = false;
static uint16_t gpu_px_x = 0;
//static uint16_t gpu_screen_x = 0;
static uint16_t gpu_total_stall = 0;
static uint16_t gpu_curr_stall = 0;
static uint8_t gpu_fetcher_px_x = 0;
static uint8_t gpu_fetcher_px_y = 0;
static bool gpu_window_y_cond = false;

static uint16_t (*gpu_get_color)(gpu_pixel_t*, gpu_pixel_t*);
static draw_f_t gpu_draw = NULL;
static mem_interface_t gpu_io;
static gpu_mode_t gpu_mode = GPU_MODE_OAM;
static bool gpu_stat_int = false;

static int8_t gpu_oam_scanline[GPU_SCREEN_WIDTH];
static uint8_t gpu_num_oam_available = GPU_NUM_OBJ_PER_SCANLINE;

// Local function declarations

static uint8_t* gpu_reg_lookup(uint16_t addr);
static inline uint16_t gpu_make_color(uint8_t color_h, uint8_t color_l);
static void gpu_render(void);
static uint16_t gpu_get_cgb_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px);
static uint16_t gpu_get_dmg_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px);
static void gpu_fetch_bg_pixels(void);
static void gpu_fetch_obj_pixels(void);
static uint8_t gpu_byte_reverse(uint8_t in);
static void gpu_transition_mode(gpu_mode_t mode);
static void gpu_scan_oam(void);

// Interface function implementations

bool gpu_init(mem_interface_t* mem_interface, bool is_cgb, draw_f_t draw) {
  gpu_io.load = mem_interface->load;
  gpu_io.store = mem_interface->store;
  gpu_is_cgb = is_cgb;
  // Color logic based on hardware
  if (is_cgb) {
    gpu_get_color = gpu_get_cgb_color;
    memset(gpu_cgb_bg_palettes, 0xFF, sizeof(gpu_cgb_bg_palettes) / sizeof(uint8_t));
  } else {
    gpu_get_color = gpu_get_dmg_color;
  }
  gpu_draw = draw;
  bool ok = cbuf_init(&gpu_bg_pixel_fifo, sizeof(gpu_pixel_t), GPU_PIXEL_BUF_SIZE);
  ok &= cbuf_init(&gpu_obj_pixel_fifo, sizeof(gpu_pixel_t), GPU_PIXEL_BUF_SIZE);
  gpu_transition_mode(GPU_MODE_OAM);
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
  // todo: lcd_y is read only
  uint8_t* reg_ptr = gpu_reg_lookup(addr);
  if (reg_ptr != NULL) {
    *reg_ptr = val;
  }
}

void gpu_run(uint8_t clocks) {
  for (uint8_t i = 0; i < clocks; i++) {
    switch (gpu_mode) {
      case GPU_MODE_OAM:
        gpu_scan_oam();
        break;
      case GPU_MODE_DRAW:
        if (gpu_curr_stall > 0) {
          gpu_curr_stall--;
          gpu_total_stall++;
        } else {
          if (cbuf_size(&gpu_bg_pixel_fifo) == 0) {
            gpu_fetch_bg_pixels();
          }
          if ((cbuf_size(&gpu_obj_pixel_fifo) == 0) && gpu_oam_scanline[gpu_px_x] != GPU_OAM_SCANLINE_NONE) {
            gpu_fetch_obj_pixels();
          }
          gpu_render();
        }
        break;
      case GPU_MODE_HBLANK:
      case GPU_MODE_VBLANK:
      default:
        break;
    }

    // y compare check
    if (gpu_lcd_y == gpu_lcd_y_compare) {
      SET_BIT(gpu_lcd_status, GPU_LCDS_LYC);
    } else {
      CLEAR_BIT(gpu_lcd_status, GPU_LCDS_LYC);
    }

    // update current dot position
    gpu_px_x++;
    if (gpu_px_x > GPU_DOT_WIDTH) {
      gpu_px_x = 0;
      gpu_lcd_y++;
    }
    if (gpu_lcd_y > GPU_DOT_HEIGHT) {
      gpu_lcd_y = 0;
    }

    // determine next gpu mode based on new dot position
    gpu_mode_t next_mode;
    if (gpu_lcd_y > GPU_DOT_VBLANK_Y) {
      next_mode = GPU_MODE_VBLANK;
    } else if (gpu_px_x <= GPU_DOT_DRAW_X_START) {
      next_mode = GPU_MODE_OAM;
    } else if (gpu_px_x <= (GPU_DOT_HBLANK_START + gpu_total_stall)) {
      next_mode = GPU_MODE_DRAW;
    } else {
      next_mode = GPU_MODE_HBLANK;
    }

    // state transition if mode changed
    if (gpu_mode != next_mode) {
      gpu_transition_mode(next_mode);
    }

    // handle stat interrupt
    bool next_stat_int = ((gpu_mode == GPU_MODE_HBLANK) && GET_BIT(gpu_lcd_status, GPU_LCDS_MODE0_INT));
    next_stat_int |= ((gpu_mode == GPU_MODE_VBLANK) && GET_BIT(gpu_lcd_status, GPU_LCDS_MODE1_INT));
    next_stat_int |= (((gpu_mode == GPU_MODE_OAM) && GET_BIT(gpu_lcd_status, GPU_LCDS_MODE2_INT)));
    next_stat_int |= (GET_BIT(gpu_lcd_status, GPU_LCDS_LYC) && GET_BIT(gpu_lcd_status, GPU_LCDS_LYC_INT_SEL));
    if (next_stat_int && !gpu_stat_int) {
      gpu_stat_int = next_stat_int;
      uint8_t int_flags = gpu_io.load(ADDR_IF);
      SET_BIT(int_flags, INT_LCD);
      gpu_io.store(ADDR_IF, int_flags);
    }
  }
}


// Local function implementations

uint8_t* gpu_reg_lookup(uint16_t addr) {
  uint8_t* reg = NULL;
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
  uint16_t color = 0;

  // Check pixel buffers
  if (cbuf_size(&gpu_bg_pixel_fifo) != 0) {
    bg_px = (gpu_pixel_t*)cbuf_read(&gpu_bg_pixel_fifo);
  }
  if (cbuf_size(&gpu_obj_pixel_fifo) != 0) {
    obj_px = (gpu_pixel_t*)cbuf_read(&gpu_obj_pixel_fifo);
  }
  if (bg_px == NULL || gpu_px_x >= GPU_SCREEN_WIDTH) {
    return;
  }
  color = gpu_get_color(bg_px, obj_px);
  gpu_draw(gpu_px_x, gpu_lcd_y, color);
}

uint16_t gpu_get_cgb_color(gpu_pixel_t* bg_px, gpu_pixel_t* obj_px) {
  uint16_t color = 0;
  bool obj_priority = false;
    if (!GET_BIT(gpu_lcd_control, GPU_LCDC_BG_WIN_EN_PRIO)) {
    obj_priority = true;
  }
  // Does the obj pixel have priority?
  if (obj_px != NULL) {
    if (obj_px->color != GPU_COLOR_INDEX_0 
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
    if (obj_px->color != GPU_COLOR_INDEX_0 
      && GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_EN)) {
      obj_priority = (obj_px->bg_priority >= bg_px->bg_priority);
    }
  }
  if (obj_priority) {
    // color based on object pixel
    uint8_t color_val_idx = 0;
    if (obj_px->palette != 0) {
      color_val_idx = (gpu_dmg_obj_palette_1 >> (obj_px->color * 2)) & 0x03U;
    } else {
      color_val_idx = (gpu_dmg_obj_palette_0 >> (obj_px->color * 2)) & 0x03U;
    }
    color = gpu_dmg_color_values[color_val_idx];
  } else {
    if (GET_BIT(gpu_lcd_control, GPU_LCDC_BG_WIN_EN_PRIO)) {
      // color based on background pixel
      uint8_t color_val_idx = (gpu_dmg_bg_palette >> (bg_px->color * 2)) & 0x03U;
      color = gpu_dmg_color_values[color_val_idx];
    } else {
      // background is disabled
      color = gpu_dmg_color_values[GPU_DMG_BG_COLOR_IDX_BLACK];
    }
  }
  return color;
}

void gpu_fetch_bg_pixels(void) {
  uint16_t window_x_start = gpu_window_x - 7; // window x offset by 7 for some reason
  uint16_t window_x_end = gpu_window_x + GPU_SCREEN_WIDTH - 7;
  uint16_t window_y_start = gpu_window_y;

  uint16_t tilemap_addr = ADDR_VRAM_TILEMAP0_START;
  if (GET_BIT(gpu_lcd_control, GPU_LCDC_BG_TILE_MAP) 
  && !RANGE_INC(gpu_px_x, window_x_start, window_x_end)) {
    tilemap_addr = ADDR_VRAM_TILEMAP1_START;
  }
  if (GET_BIT(gpu_lcd_control, GPU_LCDC_WIN_TILE_MAP)
  && RANGE_INC(gpu_px_x, window_x_start, window_x_end)) {
    tilemap_addr = ADDR_VRAM_TILEMAP1_START;
  }

  uint8_t x_coord = 0;
  if (gpu_window_y_cond && (gpu_px_x >= window_x_start)) {
    x_coord = (gpu_px_x - window_x_start) / 8;
  } else {
    x_coord = (gpu_view_x / 8 + gpu_fetcher_px_x) & 0x1F;
  }

  uint8_t y_coord = 0;
  if (gpu_window_y_cond && (gpu_px_x >= window_x_start)) {
    y_coord = (gpu_lcd_y - window_y_start) / 8;
  } else {
    y_coord = (gpu_lcd_y + gpu_view_y) & 0xFF;
  }

  // todo: check that access isn't blocked
  uint16_t tile_addr = tilemap_addr + (y_coord * GPU_TILES_PER_MAP_DIM) + x_coord;
  uint8_t tile_idx = gpu_io.load(tile_addr);

  uint16_t tile_data_addr = 0;
  if (GET_BIT(gpu_lcd_control, GPU_LCDC_BG_WIN_TILE_DATA)) {
    tile_data_addr = ADDR_VRAM_BLOCK0_START + (tile_idx * GPU_BYTES_PER_TILE) + (gpu_fetcher_px_y * 2);
  } else {
    tile_data_addr = (uint16_t)(ADDR_VRAM_BLOCK2_START + ((int16_t)tile_idx * GPU_BYTES_PER_TILE) + (gpu_fetcher_px_y * 2));
  }

  gpu_pixel_t px = {0};
  bool flip_x = false;
  if (gpu_is_cgb) {
    uint8_t tile_attr = 0;
    flip_x = GET_BIT(tile_attr, 5); // check X flip bit
    // todo: check bank bit
    px.palette = tile_attr & 0x03U;
    px.bg_priority = GET_BIT(tile_attr, 7U) ? 1U : 0U;
  } else {
    px.bg_priority = 0U;
  }

  // todo: check that vram access isn't blocked
  uint8_t tile_data_low = gpu_io.load(tile_data_addr);
  uint8_t tile_data_high = gpu_io.load(tile_data_addr + 1);
  if (flip_x) {
    tile_data_low = gpu_byte_reverse(tile_data_low);
    tile_data_high = gpu_byte_reverse(tile_data_high);
  }
  for (uint8_t i = 0; i < 8; i++) {
    // push to fifo, msb first
    uint8_t color = (tile_data_low >> (7 - i)) & 0x01U;
    color |= (tile_data_high >> (7 - i) & 0x01U) << 0x1U;
    px.color = (gpu_color_index_t)color;
    cbuf_write(&gpu_bg_pixel_fifo, &px);
  }
}

uint8_t gpu_byte_reverse(uint8_t in) {
  uint8_t reverse = in;
  uint8_t count = 7;
  in >>= 1U;
  while (in) {
    reverse <<= 1U;
    reverse |= in & 1U;
    in >>= 1U;
    count--;
  }
  reverse <<= count;
  return reverse;
}

void gpu_transition_mode(gpu_mode_t mode) {
  gpu_mode = mode;
  gpu_lcd_status &= 0xFC;
  gpu_lcd_status |= (uint8_t)mode;
  switch (mode) {
    case GPU_MODE_VBLANK:
      uint8_t int_flags = gpu_io.load(ADDR_IF);
      SET_BIT(int_flags, INT_VBLANK);
      gpu_io.store(ADDR_IF, int_flags);
      break;
    case GPU_MODE_OAM:
      memset(gpu_oam_scanline, GPU_OAM_SCANLINE_NONE, GPU_SCREEN_WIDTH);
      gpu_num_oam_available = GPU_NUM_OBJ_PER_SCANLINE;
      break;
    default:
      break;
  }
}

void gpu_scan_oam(void) {
  // handle one obj per 2 clocks since oam scan is 80
  // while there are 40 objs to scan
  if (gpu_px_x % 2 != 0) {
    return;
  }
  // check if already at max 10 objs 
  if (gpu_num_oam_available == 0) {
    return;
  }
  uint8_t obj_idx = gpu_px_x / 2;
  uint16_t obj_addr = ADDR_OAM_START + (obj_idx * GPU_BYTES_PER_OBJ);
  uint8_t obj_y = gpu_io.load(obj_addr);
  uint8_t obj_x = gpu_io.load(obj_addr + 1);
  // gross translation to screen coordinates
  uint8_t real_y = MIN((obj_y - GPU_OBJ_Y_OFFSET), 0U);
  uint8_t real_y_end = real_y + (GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_SIZE) ? MIN(obj_y, (GPU_OBJ_DOT_HEIGHT_1 - 1)) : MIN(obj_y, GPU_OBJ_DOT_HEIGHT_0 - 1));
  if (!RANGE_INC(gpu_lcd_y, real_y, real_y_end)) {
    return;
  }
  gpu_num_oam_available--;
  uint8_t real_x = MIN((obj_x - GPU_OBJ_X_OFFSET), 0U);
  uint8_t len_x = MIN(GPU_OBJ_DOT_WIDTH, obj_x);
  for (uint8_t off_x = 0; off_x < len_x; off_x++) {
    uint8_t x_pos = real_x + off_x;
    if (gpu_oam_scanline[x_pos] == GPU_OAM_SCANLINE_NONE) {
      // no obj here yet, simply set oam idx
      gpu_oam_scanline[x_pos] = obj_idx;
    } else {
      // already an obj here, existing can only be overwritten if non-cgb mode
      // and the new obj x is less than the old obj x
      if (!gpu_is_cgb) {
        uint16_t last_obj_addr = ADDR_OAM_START + (gpu_oam_scanline[x_pos] * GPU_BYTES_PER_OBJ);
        uint8_t last_obj_x = gpu_io.load(last_obj_addr + 1);
        if (obj_x < last_obj_x) {
          gpu_oam_scanline[x_pos] = obj_idx;
        }
      }
    }
  }
}

// Todo: this can be cleaned up by storing more
// info about an obj pixel calculated during oam scan
void gpu_fetch_obj_pixels(void) {
  for (uint8_t i = 0; i < 8U; i++) {
    uint8_t obj_idx = gpu_oam_scanline[gpu_px_x + i];
    if (obj_idx == GPU_OAM_SCANLINE_NONE) {
      continue;
    }
    uint16_t obj_addr = ADDR_OAM_START + (obj_idx * GPU_BYTES_PER_OBJ);
    uint8_t obj_y = gpu_io.load(obj_addr);
    uint8_t obj_x = gpu_io.load(obj_addr + 1);
    uint8_t obj_tile_idx = gpu_io.load(obj_addr + 2);
    uint8_t obj_flags = gpu_io.load(obj_addr + 3);
    gpu_pixel_t px;
    uint8_t vram_bank = 0U;
    if (gpu_is_cgb) {
      px.palette = (obj_flags & 0x07U);
      vram_bank = (GET_BIT(obj_flags, 3U) ? 1U : 0U);
    } else {
      px.palette = (GET_BIT(obj_flags, 4U) ? 1U : 0U);
    }
    px.bg_priority = (GET_BIT(obj_flags, 7U) ? 1U : 0U);
    // stupid temporary bank switch, 
    // I think real ppu has direct vram access to both banks
    uint8_t current_bank = gpu_io.load(ADDR_IO_VRAM_BANK);
    gpu_io.store(ADDR_IO_VRAM_BANK, vram_bank);
    bool flip_x = GET_BIT(obj_flags, 5U);
    bool flip_y = GET_BIT(obj_flags, 6U);
    // this is cool
    uint8_t tiles_per_obj = GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_SIZE) ? 2U : 1U;
    uint8_t obj_real_y = MIN((obj_y - GPU_OBJ_Y_OFFSET), 0U);
    uint8_t obj_y_offset = gpu_lcd_y - obj_real_y;
    if (flip_y) {
      uint8_t obj_y_relative_end = (GET_BIT(gpu_lcd_control, GPU_LCDC_OBJ_SIZE) ? (GPU_OBJ_DOT_HEIGHT_1 - 1) : (GPU_OBJ_DOT_HEIGHT_0 - 1));
      obj_y_offset = obj_y_relative_end - obj_y_offset;
    }
    uint8_t tile_addr = ADDR_VRAM_START 
                        + (obj_tile_idx * GPU_BYTES_PER_TILE * tiles_per_obj)
                        + (obj_y_offset * GPU_BYTES_PER_TILE_SLICE);
    uint8_t tile_data_low = gpu_io.load(tile_addr);
    uint8_t tile_data_high = gpu_io.load(tile_addr + 1);
    if (flip_x) {
      tile_data_low = gpu_byte_reverse(tile_data_low);
      tile_data_high = gpu_byte_reverse(tile_data_high);
    }
    uint8_t obj_real_x = MIN((obj_x - GPU_OBJ_X_OFFSET), 0U);
    uint8_t obj_x_offset = gpu_px_x + i - obj_real_x;
    uint8_t color = (tile_data_low >> (7U - obj_x_offset)) & 0x01U;
    color |= (tile_data_high >> (7U >> obj_x_offset) & 0x01U) << 0x1U;
    px.color = (gpu_color_index_t)color;
    cbuf_write(&gpu_obj_pixel_fifo, &px);
    // restore previously set vram bank
    gpu_io.store(ADDR_IO_VRAM_BANK, current_bank);
  }
}