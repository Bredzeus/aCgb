#include "timer.h"
#include "mem_map.h"

#define TIMER_CONTROL_CLK_SEL_MASK (0x3U)
#define TIMER_CONTROL_EN (2U)

typedef enum timer_clock_sel_s {
  TIMER_CLOCK_SEL_256 = 0,
  TIMER_CLOCK_SEL_4,
  TIMER_CLOCK_SEL_16,
  TIMER_CLOCK_SEL_64,
  NUM_TIMER_CLOCK_SEL
} timer_clock_sel_t;

static uint8_t timer_divider = 0; // 0xFF04
static uint8_t timer_counter = 0; // 0xFF05
static uint8_t timer_modulo = 0; // 0xFF06
static uint8_t timer_control = 0; // 0xFF07

static mem_interface_t timer_mem_io = {0};
static uint16_t timer_num_clock = 0;

static const uint16_t timer_clock_sel_val[NUM_TIMER_CLOCK_SEL] = {
  256,
  4,
  16,
  64
};

void timer_init(mem_interface_t* mem_io) {
  timer_mem_io.load = mem_io->load;
  timer_mem_io.store = mem_io->store;
}

uint8_t timer_reg_load(uint16_t addr) {
  uint8_t val = 0;
  switch (addr) {
    case ADDR_IO_DIV:
      val = timer_divider;
      break;
    case ADDR_IO_TIMA:
      val = timer_counter;
      break;
    case ADDR_IO_TMA:
      val = timer_modulo;
      break;
    case ADDR_IO_TAC:
      val = timer_control;
      break;
    default:
      break;
  }
  return val;
}

void timer_reg_store(uint16_t addr, uint8_t val) {
  switch (addr) {
    case ADDR_IO_DIV:
      timer_divider = 0;
      break;
    case ADDR_IO_TIMA:
      timer_counter = val;
      break;
    case ADDR_IO_TMA:
      timer_modulo = val;
      break;
    case ADDR_IO_TAC:
      timer_control = val;
      break;
    default:
      break;
  }
}

void timer_run(uint8_t num_clocks) {
  timer_divider += num_clocks;
  timer_num_clock += num_clocks;
  timer_clock_sel_t clock_sel = timer_control & TIMER_CONTROL_CLK_SEL_MASK;
  if (timer_num_clock >= timer_clock_sel_val[clock_sel]) {
    timer_num_clock = 0;
    timer_counter++;
    if (timer_counter == 0) {
      // timer overflowed, set to timer_modulo and request interrupt
      timer_counter = timer_modulo;
      uint8_t int_flags = timer_mem_io.load(ADDR_IF);
      SET_BIT(int_flags, INT_TIMER);
      timer_mem_io.store(ADDR_IF, int_flags);
    }
  }
}

uint8_t* timer_reg_lookup(uint16_t addr) {
  uint8_t* reg = NULL;
  switch (addr) {
    case ADDR_IO_DIV:
      reg = &timer_divider;
      break;
    case ADDR_IO_TIMA:
      reg = &timer_counter;
      break;
    case ADDR_IO_TMA:
      reg = &timer_modulo;
      break;
    case ADDR_IO_TAC:
      reg = &timer_control;
      break;
    default:
      reg = NULL;
      break;
  }
  return reg;
}