#include "mem.h"

#define MEM_DMA_TRANSFER_NUM_BYTES (160U)

static uint8_t vram[VRAM_SIZE];
static uint8_t wram0[WRAM0_SIZE];
static uint8_t wram1[WRAM1_SIZE];
static uint8_t lram[LRAM_SIZE];
static uint8_t hram[HRAM_SIZE];
static uint8_t oam[OAM_SIZE];
static uint8_t interrupt_enable = 0; // 0xFFFF
static uint8_t interrupt_flags = 0; // 0xFF0F

static uint8_t mem_dma_control_reg = 0; // 0xFF46
static uint8_t mem_dma_remaining_bytes = 0;

static mem_interface_t mem_rom_io = {0};
static mem_interface_t mem_gpu_io = {0};
static mem_interface_t mem_input_io = {0};
static mem_interface_t mem_timer_io = {0};

void mem_init(mem_interface_t *rom_io,
              mem_interface_t *gpu_io,
              mem_interface_t *input_io,
              mem_interface_t *timer_io){
  mem_rom_io.load = rom_io->load;
  mem_rom_io.store = rom_io->store;
  mem_gpu_io.load = gpu_io->load;
  mem_gpu_io.store = gpu_io->store;
  mem_input_io.load = input_io->load;
  mem_input_io.store = input_io->store;
  mem_timer_io.load = timer_io->load;
  mem_timer_io.store = timer_io->store;
}

uint8_t mem_load(uint16_t addr){
  uint8_t val = 0;
  if(addr <= ADDR_ROM1_END){
    val = mem_rom_io.load(addr);
  }else if(addr <= ADDR_VRAM_END){
    val = vram[addr - ADDR_VRAM_START];
  }else if(addr <= ADDR_EXRAM_END){
    val = mem_rom_io.load(addr);
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
    val = oam[addr - ADDR_OAM_START];
  }else if(addr <= ADDR_NU_END){
    // behavior is version dependent
    val = 0;
  }else if(addr <= ADDR_IO_END){
    val = 0;
    if (RANGE_INC(addr, ADDR_GPU0_START, ADDR_GPU0_END)
    || RANGE_INC(addr, ADDR_GPU1_START, ADDR_GPU1_END)
    || RANGE_INC(addr, ADDR_GPU2_START, ADDR_GPU2_END)) {
      val = mem_gpu_io.load(addr);
    } else if (addr == ADDR_DMA) {
      val = mem_dma_control_reg;
    } else if (addr == ADDR_IO_JOYPAD) {
      val = mem_input_io.load(addr);
    } else if (RANGE_INC(addr, ADDR_IO_DIV, ADDR_IO_TAC)) {
      val = mem_timer_io.load(addr);
    } else if (addr == ADDR_IF) {
      val = interrupt_flags;
    }
  }else if(addr <= ADDR_HRAM_END){
    val = hram[addr - ADDR_HRAM_START];
  }else{
    val = interrupt_enable;
  }
  return val;
}

void mem_store(uint16_t addr, uint8_t val){
  if(addr <= ADDR_ROM1_END){
    mem_rom_io.store(addr, val);
  }else if(addr <= ADDR_VRAM_END){
    vram[addr - ADDR_VRAM_START] = val;
  }else if(addr <= ADDR_EXRAM_END){
    mem_rom_io.store(addr, val);
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
    oam[addr - ADDR_OAM_START] = val;
  }else if(addr <= ADDR_NU_END){
    // behavior is version dependent
  }else if(addr <= ADDR_IO_END){
    if (RANGE_INC(addr, ADDR_GPU0_START, ADDR_GPU0_END)
    || RANGE_INC(addr, ADDR_GPU1_START, ADDR_GPU1_END)
    || RANGE_INC(addr, ADDR_GPU2_START, ADDR_GPU2_END)) {
      mem_gpu_io.store(addr, val);
    } else if (addr == ADDR_DMA) {
      mem_dma_control_reg = val;
      mem_dma_remaining_bytes = MEM_DMA_TRANSFER_NUM_BYTES;
    } else if (addr == ADDR_IO_JOYPAD) {
      mem_input_io.store(addr, val);
    } else if (RANGE_INC(addr, ADDR_IO_DIV, ADDR_IO_TAC)) {
      mem_timer_io.store(addr, val);
    } else if (addr == ADDR_IF) {
      interrupt_flags = val;
    }
  }else if(addr <= ADDR_HRAM_END){
    hram[addr - ADDR_HRAM_START] = val;
  }else{
    interrupt_enable = val;
  }
}

void mem_run_dma_controller(void) {
  if (mem_dma_remaining_bytes < 0) {
    uint8_t offset = MEM_DMA_TRANSFER_NUM_BYTES - mem_dma_remaining_bytes;
    uint8_t source_addr = ((uint16_t)mem_dma_control_reg << 8) | (uint16_t)offset;
    oam[offset] = mem_load(source_addr);
    mem_dma_remaining_bytes--;
  }
}