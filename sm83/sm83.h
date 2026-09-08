#ifndef SM83_H_
#define SM83_H_

#include "common.h"

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  SM83_STATE_RUN,
  SM83_STATE_HALT,
  SM83_STATE_STOP,
  NUM_SM83_STATE
} sm83_state_t;

typedef struct sm83_reg_file_s {
  uint16_t pc;
  uint16_t sp;
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint8_t ir;
  uint8_t ie;
} sm83_reg_file_t;

typedef struct sm83_io_s {
  mem_load_f_t mem_load;
  mem_store_f_t mem_store;
} sm83_io_t;

typedef struct sm83_s {
  sm83_reg_file_t reg_file;
  sm83_io_t io;
  sm83_state_t state;
  uint32_t cycles;
  bool ime; // interrupt master enable
} sm83_t;

/*
* Setup sm83 struct 
*/
void sm83_init(sm83_t* sm83, mem_load_f_t load, mem_store_f_t store);

/*
* resets the cpu
* sm83 - (sm83_t*) cpu state struct
*/
void sm83_reset(sm83_t* sm83);

/*
* executes one instruction
* sm83 - (sm83_t*) cpu state struct
* returns: (uint32_t) number of cycles passed
*/
uint32_t sm83_run(sm83_t* sm83);

void sm83_dump(sm83_t* sm83, sstring_t* str);

#endif