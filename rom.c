#include <string.h>
#include <stdlib.h>
#include "rom.h"
#include "mbc.h"

#define ROM_OFFSET_TITLE (0x0134U)
#define ROM_OFFSET_MANU_CODE (0x013FU)
#define ROM_OFFSET_CGB_FLAG (0x0143U)
#define ROM_OFFSET_NEW_LICENSEE (0x0144U)
#define ROM_OFFSET_SGB_FLAG (0x0146U)
#define ROM_OFFSET_CART_TYPE (0x0147U)
#define ROM_OFFSET_ROM_SIZE (0x0148U)
#define ROM_OFFSET_RAM_SIZE (0x0149U)
#define ROM_OFFSET_DEST_CODE (0x014AU)
#define ROM_OFFSET_OLD_LICENSEE (0x014BU)
#define ROM_OFFSET_VERSION (0x014CU)
#define ROM_OFFSET_HEADER_CHKSUM (0x014DU)
#define ROM_OFFSET_GLOBAL_CHKSUM (0x014EU)
#define ROM_END_HEADER (ROM_OFFSET_GLOBAL_CHKSUM + ROM_GLOBAL_CHKSUM_SIZE)

static bool rom_info_init(rom_info_t* rom_info);
static uint8_t rom_load_(size_t pos);
static void rom_store_(size_t pos, uint8_t val);
static mbc_type_t rom_map_type_to_mbc(uint32_t rom_type);

#ifdef GB_HOST_HAS_FILESYSTEM
#include <stdio.h>
static bool rom_save_byte_to_file(size_t pos, uint8_t val);
static bool rom_init_ram(void);
static bool rom_generate_ram_filepath(void);
static char* rom_filepath = NULL;
static char* ram_filepath = NULL;
#endif /* GB_HOST_HAS_FILESYSTEM */

static uint8_t* rom_data = NULL;
static size_t rom_data_size = 0; // in bytes
static bool rom_ready = false;
static rom_info_t rom_info;
static uint8_t* ram_data = NULL;

bool rom_info_init(rom_info_t* rom_info) {
  if (!rom_ready){
    return false;
  }
  memcpy(rom_info->title, &rom_data[ROM_OFFSET_TITLE], ROM_TITLE_SIZE);
  memcpy(rom_info->manufacturer, &rom_data[ROM_OFFSET_MANU_CODE], ROM_MANU_CODE_SIZE);
  rom_info->cgb_flag = rom_data[ROM_OFFSET_CGB_FLAG];
  memcpy(rom_info->new_licensee, &rom_data[ROM_OFFSET_NEW_LICENSEE], ROM_LICENSEE_SIZE);
  rom_info->sgb_flag = rom_data[ROM_OFFSET_SGB_FLAG];
  rom_info->cart_type = rom_data[ROM_OFFSET_CART_TYPE];
  rom_info->rom_size = rom_data[ROM_OFFSET_ROM_SIZE];
  rom_info->ram_size = rom_data[ROM_OFFSET_RAM_SIZE];
  rom_info->destination = rom_data[ROM_OFFSET_DEST_CODE];
  rom_info->old_licensee = rom_data[ROM_OFFSET_OLD_LICENSEE];
  rom_info->version = rom_data[ROM_OFFSET_VERSION];
  rom_info->header_checksum = rom_data[ROM_OFFSET_HEADER_CHKSUM];
  memcpy(rom_info->global_checksum, &rom_data[ROM_OFFSET_GLOBAL_CHKSUM], ROM_GLOBAL_CHKSUM_SIZE);
  return true;
}

uint8_t rom_load_(size_t pos) {
  if (!rom_ready || pos > rom_data_size) {
    return 0;
  }
  return rom_data[pos];
}

bool rom_store_(size_t pos, uint8_t val) {
  if (!rom_ready || pos > rom_data_size) {
    return false;
  }
  rom_data[pos] = val;
  return rom_save_byte_to_file(pos, val);
}

void rom_cleanup(void) {
  if (rom_data != NULL) {
    free(rom_data);
    rom_data = NULL;
  }
  if (ram_data != NULL) {
    free(ram_data);
    ram_data = NULL;
  }
  rom_data_size = 0;
  if (rom_filepath != NULL) {
    free(rom_filepath);
    rom_filepath = NULL;
  }
  if (ram_filepath != NULL) {
    free(ram_filepath);
    ram_filepath = NULL;
  }
  rom_ready = false;
  memset(rom_info, 0, sizeof(rom_info_t));
}

uint8_t rom_load(uint16_t addr) {
  return mbc_load(addr);
}

void rom_store(uint16_t addr, uint8_t val) {
  mbc_store(addr, val);
}

void rom_get_info(rom_info_t* info) {
  memcpy(info, &rom_info, sizeof(rom_info_t));
}

mbc_type_t rom_map_type_to_mbc(uint32_t rom_type) {
  mbc_type_t mbc_type = MBC_TYPE_NONE;
  switch(rom_type) {
    case ROM_CART_ROM_ONLY:
      mbc_type = MBC_TYPE_NONE;
      break;
    case ROM_CART_MBC1:
    case ROM_CART_MBC1_RAM:
    case ROM_CART_MBC1_RAM_BATT:
      mbc_type = MBC_TYPE_MBC1;
      break;
    case ROM_CART_MBC2:
    case ROM_CART_MBC2_BATT:
      mbc_type = MBC_TYPE_MBC2;
      break;
    case ROM_CART_MMM01:
    case ROM_CART_MMM01_RAM:
    case ROM_CART_MMM01_RAM_BATT:
      mbc_type = MBC_TYPE_MMM01;
      break;
    case ROM_CART_MBC3:
    case ROM_CART_MBC3_TIMER_BATT:
    case ROM_CART_MBC3_TIMER_RAM_BATT:
    case ROM_CART_MBC3_RAM:
    case ROM_CART_MBC3_RAM_BATT:
      mbc_type = MBC_TYPE_MBC3;
      break;
    case ROM_CART_MBC5:
    case ROM_CART_MBC5_RAM:
    case ROM_CART_MBC5_RAM_BATT:
    case ROM_CART_MBC5_RUMBLE:
    case ROM_CART_MBC5_RUMBLE_RAM:
    case ROM_CART_MBC5_RUMB_RAM_BATT:
      mbc_type = MBC_TYPE_MBC5;
      break;
    case ROM_CART_MBC6:
      mbc_type = MBC_TYPE_MBC6;
      break;
    case ROM_CART_MBC7_SENS_RUMB_RAM_BATT:
      mbc_type = MBC_TYPE_MBC7;
      break;
    case ROM_CART_HUC3:
      mbc_type = MBC_TYPE_HUC3;
      break;
    case ROM_CART_HUC1_RAM_BATT:
      mbc_type = MBC_TYPE_HUC1;
      break;
    case ROM_CART_ROM_RAM:
    case ROM_CART_ROM_RAM_BATT:
    case ROM_CART_CAMERA:
    case ROM_CART_BANDAITAMA5:
    default:
      mbc_type = MBC_TYPE_OTHER;
      break;
  }
  return mbc_type;
}

#ifdef GB_HOST_HAS_FILESYSTEM
bool rom_save_byte_to_file(size_t pos, uint8_t val) {
  if (!rom_ready || pos > rom_data_size) {
    return false;
  }
  FILE* file;
  file = fopen(ram_filepath, "wb");
  fseek(file, pos, SEEK_SET);
  fwrite(&val, sizeof(uint8_t), 1, file);
  fclose(file);
  return true;
}

bool rom_load_from_file(char* filepath) {
  if (rom_ready) {
    rom_cleanup();
  }
  FILE* file;
  file = fopen(filepath, "rb");
  if (file == NULL) {
    return false;
  }
  fseek(file, 0L, SEEK_END);
  rom_data_size = (size_t)ftell(file);
  rom_data = (uint8_t*)malloc(sizeof(uint8_t) * rom_data_size);
  if (rom_data == NULL) {
    rom_data_size = 0;
    fclose(file);
    return false;
  }
  fseek(file, 0L, SEEK_SET);
  size_t num_read = fread(rom_data, sizeof(uint8_t), rom_data_size, file);
  fclose(file);
  rom_ready = (num_read == rom_data_size);
  size_t filepath_len = strlen(filepath);
  rom_filepath = (char*)calloc(sizeof(char) * filepath_len);
  memcpy(rom_filepath, filepath, filepath_len);
  rom_info_init(&rom_info);
  mbc_type_t mbc_type = rom_map_type_to_mbc(rom_info.cart_type);
  mbc_init(mbc_type, rom_load_, rom_store_);
  rom_ready &= rom_init_ram();
  return rom_ready;
}

bool rom_init_ram(void) {
  bool init_ok = true;
  size_t ram_size_bytes = 0;
  switch (rom_info.ram_size) {
    case ROM_RAM_SIZE_8KB:
      ram_size_bytes = 8192;
      break;
    case ROM_RAM_SIZE_32KB:
      ram_size_bytes = 32768;
      break;
    case ROM_RAM_SIZE_128KB:
      ram_size_bytes = 131072;
      break;
    case ROM_RAM_SIZE_64KB:
      ram_size_bytes = 65536;
      break;
    case ROM_RAM_SIZE_NONE:
    case ROM_RAM_SIZE_INV:
    default:
      break;
  }
  if (ram_size_bytes != 0) {
    ram_data = (uint8_t*)calloc(ram_size_bytes, sizeof(uint8_t));
    if (ram_data == NULL) {
      init_ok = false;
    }
    init_ok &= rom_generate_ram_filepath();
  }
  return init_ok;
}

bool rom_generate_ram_filepath(void) {
  const char[] gb_extension = ".gb";
  const char[] sav_extension = ".sav";
  const size_t sav_len = sizeof(sav_extension) / sizeof(char);
  const char* rom_gb_ext = strstr(rom_filepath, gb_extension);
  if (rom_gb_ext == NULL) {
    return false;
  }
  const size_t name_len = rom_gb_ext - rom_filepath;
  ram_filepath = (char*)malloc((name_len + sav_len), sizeof(char));
  if (ram_filepath == NULL) {
    return false;
  }
  memcpy(ram_filepath, rom_fp, name_len);
  memcpy(ram_filepath, sav_extension, sav_len);
  return true;
}

#endif /* GB_HOST_HAS_FILESYSTEM */
