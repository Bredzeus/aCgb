#ifndef ROM_H_
#define ROM_H_

#include <stdint.h>

#define ROM_TITLE_SIZE (16U)
#define ROM_MANU_CODE_SIZE (4U)
#define ROM_LICENSEE_SIZE (2U)
#define ROM_GLOBAL_CHKSUM_SIZE (2U)

#define ROM_CGB_MONO (0x80U) // Supports monochrome mode
#define ROM_CGB_ONLY (0xC0U) // Supports color mode

// Todo: add new licensee codes

#define ROM_SGB_EN (0x03U) // Supports super gameboy functionality

#define ROM_CART_ROM_ONLY (0x00U)
#define ROM_CART_MBC1 (0x01U)
#define ROM_CART_MBC1_RAM (0x02U)
#define ROM_CART_MBC1_RAM_BATT (0x03U)
#define ROM_CART_MBC2 (0x05U)
#define ROM_CART_MBC2_BATT (0x06U)
#define ROM_CART_ROM_RAM (0x08U) // Behavior undefined
#define ROM_CART_ROM_RAM_BATT (0x09U) // Behavior undefined
#define ROM_CART_MMM01 (0x0BU)
#define ROM_CART_MMM01_RAM (0x0CU)
#define ROM_CART_MMM01_RAM_BATT (0x0DU)
#define ROM_CART_MBC3_TIMER_BATT (0x0FU)
#define ROM_CART_MBC3_TIMER_RAM_BATT (0x10U)
#define ROM_CART_MBC3 (0x11U)
#define ROM_CART_MBC3_RAM (0x12U)
#define ROM_CART_MBC3_RAM_BATT (0x13U)
#define ROM_CART_MBC5 (0x19U)
#define ROM_CART_MBC5_RAM (0x1AU)
#define ROM_CART_MBC5_RAM_BATT (0x1BU)
#define ROM_CART_MBC5_RUMBLE (0x1CU)
#define ROM_CART_MBC5_RUMBLE_RAM (0x1DU)
#define ROM_CART_MBC5_RUMB_RAM_BATT (0x1EU)
#define ROM_CART_MBC6 (0x20U)
#define ROM_CART_MBC7_SENS_RUMB_RAM_BATT (0x22U)
#define ROM_CART_CAMERA (0xFCU)
#define ROM_CART_BANDAITAMA5 (0xFDU)
#define ROM_CART_HUC3 (0xFEU)
#define ROM_CART_HUC1_RAM_BATT (0xFFU)

// Each rom bank is 16KB
#define ROM_SIZE_32KB (0x00U)
#define ROM_SIZE_64KB (0x01U)
#define ROM_SIZE_128KB (0x02U)
#define ROM_SIZE_256KB (0x03U)
#define ROM_SIZE_512KB (0x04U)
#define ROM_SIZE_1MB (0x05U)
#define ROM_SIZE_2MB (0x06U)
#define ROM_SIZE_4MB (0x07U)
#define ROM_SIZE_8MB (0x08U)
#define ROM_SIZE_1PT1MB (0x52U) // 72 banks
#define ROM_SIZE_1PT2MB (0x53U) // 80 banks
#define ROM_SIZE_1PT5MB (0x54U) // 96 banks

// Each ram bank is 8KB
#define ROM_RAM_SIZE_NONE (0x00U)
#define ROM_RAM_SIZE_INV (0x01U)
#define ROM_RAM_SIZE_8KB (0x02U)
#define ROM_RAM_SIZE_32KB (0x03U)
#define ROM_RAM_SIZE_128KB (0x04U)
#define ROM_RAM_SIZE_64KB (0x05U)

#define ROM_DEST_JP (0x00U)
#define ROM_DEST_OTHER (0x01U)

// Todo: add old licensee codes

typedef struct rom_info_s {
  uint8_t title[ROM_TITLE_SIZE];
  uint8_t manufacturer[ROM_MANU_CODE_SIZE]; // not always present
  uint8_t cgb_flag; // not always present
  uint8_t new_licensee[ROM_LICENSEE_SIZE];
  uint8_t sgb_flag;
  uint8_t cart_type;
  uint8_t rom_size; // not the actual size, check defines
  uint8_t ram_size; // not the actual size, check defines
  uint8_t destination;
  uint8_t old_licensee;
  uint8_t version;
  uint8_t header_checksum;
  uint8_t global_checksum[ROM_GLOBAL_CHKSUM_SIZE]; // big endian
} rom_info_t;

/*
* Cleans up internal data structures for rom, should be done on program exit
*/
void rom_cleanup(void);
/*
* loads a byte from rom
* a rom must have already been loaded
*/
uint8_t rom_load(uint16_t addr);
/*
* Stores a byte into rom
* a rom must have already been loaded
*/
void rom_store(uint16_t addr, uint8_t val);
/*
*  Gets rom info
*/
void rom_get_info(rom_info_t* info);

#ifdef GB_HOST_HAS_FILESYSTEM
/*
* Load rom from file
*/
bool rom_load_from_file(char* filepath);
#endif /* GB_HOST_HAS_FILESYSTEM */

#endif /* ROM_H_ */