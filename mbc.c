#include "mbc.h"
#include "mem_map.h"

#define MBC1_RAM_ENABLE_END (0x1FFFU)
#define MBC1_ROM_BANK_NUM_END (0x3FFFU)
#define MBC1_RAM_BANK_NUM_END (0x5FFFU)
#define MBC1_BANK_MODE_END (0x7FFFU)

#define MBC2_RAM_SIZE (0x1FFU)
#define MBC2_RAM_ENABLE_END (0x3FFFU)
#define MBC2_CTRL_SEL_MASK (0x1U << 8)

#define MBC3_RTC_SECONDS (0x08U)
#define MBC3_RTC_MINUTES (0x09U)
#define MBC3_RTC_HOURS (0x0AU)
#define MBC3_RTC_DAY_LOW (0x0BU)
#define MBC3_RTC_DAY_HIGH (0x0CU)
#define MBC3_RAM_ENABLE_END (0x1FFFU)
#define MBC3_ROM_BANK_NUM_END (0x3FFFU)
#define MBC3_RAM_BANK_NUM_END (0x5FFFU)
#define MBC3_CLOCK_LATCH_END (0x7FFFU)

// Static function defs
static void mbc_init_none(void);
static uint8_t mbc_load_none(uint16_t addr);
static void mbc_load_none(uint16_t addr, uint8_t val);

static void mbc_init_mbc1(void);
static uint8_t mbc_load_mbc1(uint16_t addr);
static void mbc_store_mbc1(uint16_t addr, uint8_t val);

static void mbc_init_mbc2(void);
static uint8_t mbc_load_mbc2(uint16_t addr);
static void mbc_store_mbc2(uint16_t addr, uint8_t val);

static void mbc_init_mbc3(void);
static uint8_t mbc_load_mbc3(uint16_t addr);
static void mbc_store_mbc3(uint16_t addr, uint8_t val);

static void mbc_init_mbc5(void);
static uint8_t mbc_load_mbc5(uint16_t addr);
static void mbc_store_mbc5(uint16_t addr, uint8_t val);

static void mbc_init_mbc6(void);
static uint8_t mbc_load_mbc6(uint16_t addr);
static void mbc_store_mbc6(uint16_t addr, uint8_t val);

static void mbc_init_mbc7(void);
static uint8_t mbc_load_mbc7(uint16_t addr);
static void mbc_store_mbc7(uint16_t addr, uint8_t val);

static void mbc_init_mmm01(void);
static uint8_t mbc_load_mmm01(uint16_t addr);
static void mbc_store_mmm01(uint16_t addr, uint8_t val);

static void mbc_init_m161(void);
static uint8_t mbc_load_m161(uint16_t addr);
static void mbc_store_m161(uint16_t addr, uint8_t val);

static void mbc_init_huc1(void);
static uint8_t mbc_load_huc1(uint16_t addr);
static void mbc_store_huc1(uint16_t addr, uint8_t val);

static void mbc_init_huc3(void);
static uint8_t mbc_load_huc3(uint16_t addr);
static void mbc_store_huc3(uint16_t addr, uint8_t val);

static void mbc_init_other(void);
static uint8_t mbc_load_other(uint16_t addr);
static void mbc_store_other(uint16_t addr, uint8_t val);

static rom_load_f_t rom_load_cb = NULL;
static rom_store_f_t ram_store_cb = NULL;
static rom_load_f_t ram_load_cb = NULL;
static mem_load_f_t mbc_load_ = NULL;
static mem_store_f_t mbc_store_ = NULL;
static mbc_type_t mbc_type = MBC_TYPE_NONE;
static mbc_t mbc;

// API func impl
void mbc_init(mbc_type_t type, rom_load_f_t rom_load, rom_store_f_t ram_store, rom_load_f_t ram_load) {
  switch (type) {
    case MBC_TYPE_NONE:
      mbc_init_none();
      break;
    case MBC_TYPE_MBC1:
      mbc_init_mbc1();
      break;
    case MBC_TYPE_MBC2:
      mbc_init_mbc2();
      break;
    case MBC_TYPE_MBC3:
      mbc_init_mbc3();
      break;
    case MBC_TYPE_MBC5:
      mbc_init_mbc5();
      break;
    case MBC_TYPE_MBC6:
      mbc_init_mbc6();
      break;
    case MBC_TYPE_MBC7:
      mbc_init_mbc7();
      break;
    case MBC_TYPE_MMM01:
      mbc_init_mmm01();
      break;
    case MBC_TYPE_M161:
      mbc_init_m161();
      break;
    case MBC_TYPE_HUC1:
      mbc_init_huc1();
      break;
    case MBC_TYPE_HUC3:
      mbc_init_huc3();
      break;
    case MBC_TYPE_OTHER:
    default:
      mbc_init_other();
      break;
  }
  mbc_type = type;
  rom_load_cb = rom_load;
  ram_store_cb = ram_store;
  ram_load_cb = ram_load;
}

uint8_t mbc_load(uint16_t addr) {
  return mbc_load_(addr);
}

void mbc_store(uint16_t addr, uint8_t val) {
  mbc_store_(addr, val);
}

// Static function impl
/*
* No mapper chip
* 32KiB mapped directly into 0x0000 - 0x7FFF
*/
void mbc_init_none(void) {
  mbc_load = mbc_load_none;
  mbc_store = mbc_store_none;
}

uint8_t mbc_load_none(uint16_t addr) {
  uint8_t val = 0;
  if (addr <= ADDR_ROM1_END) {
    val = rom_load((size_t)addr);
  }
  return val;
}

// No ram to store
void mbc_store_none(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

/*
* MBC1
* Ain't taking notes here, see: https://gbdev.io/pandocs/MBC1.html
*/
void mbc_init_mbc1(void) {
  mbc_load_ = mbc_load_mbc1;
  mbc_store_ = mbc_store_mbc1;
  mbc.mbc1.ram_en = false;
  mbc.mbc1.rom_bank = 1;
  mbc.mbc1.ram_bank = 0;
  mbc.mbc1.bank_mode = false;
}

uint8_t mbc_load_mbc1(uint16_t addr) {
  size_t rom_addr = 0;
  uint8_t val = 0;
  if (addr <= ADDR_ROM0_END) {
    rom_addr = addr & 0x3FU;
    if (mbc.mbc1.ram_en && mbc.mbc1.bank_mode) {
      rom_addr |= (mbc.mbc1.ram_bank << 19);
    }
    val = rom_load_cb(rom_addr);
  }else if (addr <= ADDR_ROM1_END) {
    uint8_t rom_bank = mbc.mbc1.rom_bank;
    // Rom bank 0 maps to 1
    if (rom_bank == 0) {
      rom_bank = 1;
    }
    rom_addr = (addr & 0x3FFFU) | (rom_bank << 14) | (mbc.mbc1.ram_bank << 19);
    val = rom_load_cb(rom_addr);
  }else if (addr >= ADDR_EXRAM_START && addr <= ADDR_EXRAM_END) {
    if (mbc.mbc1.ram_en) {
      if (mbc.mcb1.bank_mode) {
        rom_addr = addr & 0x3FFF;
      }else {
        rom_addr = (addr & 0x3FFF) | ((ram_bank & 0x02) << 13);
      }
      val = ram_load_cb(rom_addr);
    }else {
      val = 0xFF;
    }
  }
  return val;
}

void mbc_store_mbc1(uint16_t addr, uint8_t val) {
  bool write_ram = false;
  if (addr <= MBC1_RAM_ENABLE_END) {
    if (val & 0x0F == 0x0A) {
      mbc.mbc1.ram_en = true;
    }else {
      mbc.mbc1.ram_en = false;
    }
  }else if (addr <= MBC1_ROM_BANK_NUM_END) {
    mbc.mbc1.rom_bank = val;
  }else if (addr <= MBC1_RAM_BANK_NUM_END) {
    mbc.mbc1.ram_bank = val;
  }else if (addr <= MBC1_BANK_MODE_END) {
    mbc.mbc1.bank_mode = (bool)val;
  }else {
    write_ram = mbc.mbc1.ram_en;
  }
  if (write_ram) {
    size_t rom_addr = 0;
    if (mbc.mcb1.bank_mode) {
      rom_addr = addr & 0x3F;
    }else {
      rom_addr = (addr & 0x3F) | ((ram_bank & 0x02) << 13);
    }
    ram_store_cb(rom_addr, val);
  }
}

/*
* MBC2
*/
void mbc_init_mbc2(void) {
  mbc_load_ = mbc_load_mbc2;
  mbc_store_ = mbc_store_mbc2;
  mbc.mbc2.ram_en = false;
  mbc.mbc2.rom_bank = 1;
}

uint8_t mbc_load_mbc2(uint16_t addr) {
  size_t rom_addr = 0;
  uint8_t val = 0;
  if (addr <= ADDR_ROM0_END) {
    rom_addr = addr;
    val = rom_load_cb(rom_addr);
  } else if (addr <= ADDR_ROM1_END) {
    uint8_t rom_bank = mbc.mbc2.rom_bank;
    // Rom bank 0 maps to 1
    if (rom_bank == 0) {
      rom_bank = 1;
    }
    rom_addr = (addr & 0x3FU) | (rom_bank << 14);
    val = rom_load_cb(rom_addr);
  } else if (addr >= ADDR_EXRAM_START && addr <= ADDR_EXRAM_END && mbc.mbc2.ram_en) {
    // mbc2 has 512 (half-bytes) of ram that are mirrored through external ram space
    uint16_t addr_ram = (addr - ADDR_EXRAM_START) % MBC2_RAM_SIZE;
    val = ram_load_cb(addr_ram);
  }
  return val;
}

void mbc_store_mbc2(uint16_t addr, uint8_t val) {
  if (addr <= MBC2_RAM_ENABLE_END) {
    if (addr & MBC2_CTRL_SEL_MASK) {
      mbc.mbc2.rom_bank = val;
    } else {
      mbc.mbc2.ram_en = ((val & 0xA) == 0xA);
    }
  } else if (addr >= ADDR_EXRAM_START && addr <= ADDR_EXRAM_END && mbc.mbc2.ram_en) {
    uint16_t addr_ram = (addr - ADDR_EXRAM_START) % MBC2_RAM_SIZE;
    ram_store_cb(addr_ram, val);
  }
}

/*
* MBC3
*/
void mbc_init_mbc3(void) {
  mbc_load_ = mbc_load_mbc3;
  mbc_store_ = mbc_store_mbc3;
  mbc.mbc3.rtc = 0;
  mbc.mbc3.ram_timer_en = false;
  mbc.mbc3.rom_bank = 1;
  mbc.mbc3.ram_bank = 0;
  mbc.mbc3.latch_rtc = false;
}

uint8_t mbc_load_mbc3(uint16_t addr) {
  size_t rom_addr = 0;
  uint8_t val = 0;
  if (addr <= ADDR_ROM0_END) {
    val = rom_load_cb((size_t)addr);
  } else if (addr <= ADDR_ROM1_END) {
    uint8_t rom_bank = mbc.mbc3.rom_bank;
    // Rom bank 0 maps to 1
    if (rom_bank == 0) {
      rom_bank = 1;
    }
    rom_addr = (addr & 0x3FU) | (rom_bank << 14);
    val = rom_load_cb(rom_addr);
  } else if (mbc.mbc3.ram_timer_en){
    if (mbc.mbc3.ram_bank < 0x8U) {
      rom_addr = (size_t)(addr - ADDR_EXRAM_START) | ((size_t)mbc.mbc3.ram_bank << 16);
      val = ram_load_cb(rom_addr);
    } else {
      // TODO: update rtc if not latched
      switch (mbc.mbc3.ram_bank) {
        case MBC3_RTC_SECONDS:
          val = mbc.mbc3.rtc.seconds;
          break;
        case MBC3_RTC_MINUTES:
          val = mbc.mbc3.rtc.minutes;
          break;
        case MBC3_RTC_HOURS:
          val = mbc.mbc3.rtc.hours;
          break;
        case MBC3_RTC_DAY_LOW:
          val = mbc.mbc3.rtc.day_low;
          break;
        case MBC3_RTC_DAY_HIGH:
          val = mbc.mbc3.rtc.day_high;
          break;
        default:
          break;
      }
    }
  }
  return val;
}

void mbc_store_mbc3(uint16_t addr, uint8_t val) {
  if (addr <= MBC3_RAM_ENABLE_END) {
    mbc.mbc3.ram_timer_en = (val == 0x0A);
  } else if (addr <= MBC3_ROM_BANK_NUM_END) {
    mbc.mbc3.rom_bank = val;
  } else if (addr <= MBC3_RAM_BANK_NUM_END) {
    mbc.mbc3.ram_bank = val;
  } else if (addr <= MBC3_CLOCK_LATCH_END) {
    if (!mbc.mbc3.latch_rtc && (val == 0x01)) {
      // Todo: latch rtc
    }
    mbc.mbc3.latch_rtc = val;
  } else if (mbc.mbc3.ram_timer_en) {
    if (mbc.mbc3.ram_bank < 0x8U) {
      size_t ram_addr = (size_t)(addr - ADDR_EXRAM_START) | ((size_t)mbc.mbc3.ram_bank << 16)
      ram_store_cb(ram_addr, val);
    } else {
      // Todo: rtc
    }
  }
}


void mbc_init_mbc5(void) {
  mbc_load_ = mbc_load_mbc5;
  mbc_store_ = mbc_store_mbc5;
  mbc.mbc5.ram_en = false;
  mbc.mbc5.rom_bank = 1;
  mbc.mbc5.ram_bank = 0;
}

uint8_t mbc_load_mbc5(uint16_t addr) {
  (void)addr;
  return 0;
}
void mbc_store_mbc5(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

void mbc_init_mbc6(void) {
  mbc_load_ = mbc_load_mbc6;
  mbc_store_ = mbc_store_mbc6;
}

uint8_t mbc_load_mbc6(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_mbc6(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

void mbc_init_mbc7(void) {
  mbc_load_ = mbc_load_mbc7;
  mbc_store_ = mbc_store_mbc7;
}

uint8_t mbc_load_mbc7(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_mbc7(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

void mbc_init_mmm01(void) {
  mbc_load_ = mbc_load_mmm01;
  mbc_store_ = mbc_store_mmm01;
}

uint8_t mbc_load_mmm01(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_mmm01(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

void mbc_init_m161(void) {
  mbc_load_ = mbc_load_m161;
  mbc_store_ = mbc_store_m161;
}

uint8_t mbc_load_m161(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_m161(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

void mbc_init_huc1(void) {
  mbc_load_ = mbc_load_huc1;
  mbc_store_ = mbc_store_huc1;
}

uint8_t mbc_load_huc1(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_huc1(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val
}

void mbc_init_huc3(void) {
  mbc_load_ = mbc_load_huc3;
  mbc_store_ = mbc_store_huc3;
}

uint8_t mbc_load_huc3(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_huc3(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}

void mbc_init_other(void) {
  mbc_load_ = mbc_load_other;
  mbc_store_ = mbc_store_other;
}

uint8_t mbc_load_other(uint16_t addr) {
  (void)addr;
  return 0;
}

void mbc_store_other(uint16_t addr, uint8_t val) {
  (void)addr;
  (void)val;
}