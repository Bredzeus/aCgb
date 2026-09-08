#include "mem.h"

typedef struct mem_io_s {
  bool todo;
} mem_io_t;

static uint8_t vram[VRAM_SIZE];
static uint8_t wram0[WRAM0_SIZE];
static uint8_t wram1[WRAM1_SIZE];
static uint8_t lram[LRAM_SIZE];
static uint8_t hram[HRAM_SIZE];
static uint8_t oam[OAM_SIZE];
static uint8_t interrupt_enable;

static mem_load_f_t rom_load_ = NULL;
static mem_store_f_t rom_store_ = NULL;

void mem_init(mem_load_f_t rom_load, mem_store_f_t rom_store){
  rom_load_ = rom_load;
  rom_store_ = rom_store;
}

uint8_t mem_load(uint16_t addr){
  uint8_t val = 0;
  if(addr <= ADDR_ROM1_END){
    val = mbc_load_(addr);
  }else if(addr <= ADDR_VRAM_END){
    val = vram[addr - ADDR_VRAM_START];
  }else if(addr <= ADDR_EXRAM_END){
    val = mbc_load_(addr);
  }else if(addr <= ADDR_WRAM0_END){
    val = wram0[addr - ADDR_WRAM0_START];
  }else if(addr <= ADDR_WRAM1_END){
    val = wram1[addr - ADDR_WRAM1_START];
  }else if(addr <= ADDR_LRAM_END){
    val = lram[addr - ADDR_LRAM_START];
  }else if(addr <= ADDR_ERAM_END){
    // Echo RAM (mirrors 0xC000 - 0xDDFF)
    val = mem_load((addr - ADDR_ERAM_START) + ADDR_WRAM0_START);
  }else if(addr <= ADDR_OAM_END){
    // might need to be mapped to the gpu
    val = oam[addr - ADDR_OAM_START];
  }else if(addr <= ADDR_NU_END){
    // behavior is version dependent
    val = 0;
  }else if(addr <= ADDR_IO_END){
    // TODO: io, split up into its individual components, no point in trying to combine everything
  }else if(addr <= ADDR_HRAM_END){
    val = hram[addr - ADDR_HRAM_START];
  }else{
    val = interrupt_enable;
  }
  return val;
}

void mem_store(uint16_t addr, uint8_t val){
  if(addr <= ADDR_ROM1_END){
    mbc_store_(addr, val);
  }else if(addr <= ADDR_VRAM_END){
    vram[addr - ADDR_VRAM_START] = val;
  }else if(addr <= ADDR_EXRAM_END){
    mbc_store_(addr, val);
  }else if(addr <= ADDR_WRAM0_END){
    wram0[addr - ADDR_WRAM0_START] = val;
  }else if(addr <= ADDR_WRAM1_END){
    wram1[addr - ADDR_WRAM1_START] = val;
  }else if(addr <= ADDR_LRAM_END){
    lram[addr - ADDR_LRAM_START] = val;
  }else if(addr <= ADDR_ERAM_END){
    // Echo RAM (mirrors 0xC000 - 0xDDFF)
    mem_store((addr - ADDR_ERAM_START) + ADDR_WRAM0_START, val);
  }else if(addr <= ADDR_OAM_END){
    // might need to be mapped to the gpu
    oam[addr - ADDR_OAM_START] = val;
  }else if(addr <= ADDR_NU_END){
    // behavior is version dependent
  }else if(addr <= ADDR_IO_END){
    // TODO: io, split up into its individual components, no point in trying to combine everything
  }else if(addr <= ADDR_HRAM_END){
    hram[addr - ADDR_HRAM_START] = val;
  }else{
    interrupt_enable = val;
  }
}