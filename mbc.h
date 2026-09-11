#ifndef MBC_H_
#define MBC_H_

#include <stdbool.h>

#include "common.h"

typedef enum {
  MBC_TYPE_NONE,
  MBC_TYPE_MBC1,
  MBC_TYPE_MBC2,
  MBC_TYPE_MBC3,
  MBC_TYPE_MBC5,
  MBC_TYPE_MBC6,
  MBC_TYPE_MBC7,
  MBC_TYPE_MMM01,
  MBC_TYPE_M161,
  MBC_TYPE_HUC1,
  MBC_TYPE_HUC3,
  MBC_TYPE_OTHER,
  NUM_MBC_TYPE
} mbc_type_t;

typedef struct mbc1_s {
  bool ram_en;
  uint8_t rom_bank;
  uint8_t ram_bank;
  bool bank_mode;
} mbc1_t;

#define MBC_MBC2_RAM_SIZE (512U)
typedef struct mbc2_s {
  bool ram_en;
  uint8_t rom_bank;
} mbc2_t;

typedef struct mbc3_rtc_s {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t days_low;
  uint8_t days_high;
} mbc3_rtc_t;

typedef struct mbc3_s {
  mbc3_rtc_t rtc;
  bool ram_timer_en;
  uint8_t rom_bank;
  uint8_t ram_bank;
  bool latch_rtc;
} mbc3_t;

typedef struct mbc5_s {
  bool ram_en;
  uint16_t rom_bank;
  uint8_t ram_bank;
} mbc5_t;

typedef struct mbc6_s {
  bool ram_en;
  uint8_t ram_bank_a;
  uint8_t ram_bank_b;
  bool flash_en;
  bool flash_wr_en;
  uint8_t bank_a;
  bool bank_a_sel;
  uint8_t bank_b;
  bool bank_b_sel;
  // Todo flash commands
} mbc6_t;

// todo other mbcs
typedef struct mbc7_s {
  bool todo;
} mbc7_t;

typedef union mbc_s {
  mbc1_t mbc1;
  mbc2_t mbc2;
  mbc3_t mbc3;
  mbc5_t mbc5;
  mbc6_t mbc6;
  mbc7_t mbc7;
} mbc_t;

void mbc_init(mbc_type_t type, rom_load_f_t rom_load, rom_store_f_t ram_store, rom_load_f_t ram_load);
uint8_t mbc_load(uint16_t addr);
void mbc_store(uint16_t addr, uint8_t val);
// Todo: add mbc_update to handle mbcs with special functions, i.e. rtc

#endif