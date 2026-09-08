#include "gb.h"
#include <stdbool.h>

#include "common.h"

typedef enum {
  GB_STATE_READY = 0,
  GB_STATE_RUNNING,
  GB_STATE_PAUSED,
  NUM_GB_STATE
} gb_state_t;

//TODO: decide how memory is gonna work
//
// 1. should we just write to a big block of memory
// then each module will know its own addresses
// and manage everything on their own update cycles.
// Pros: simpler (probably), can handle clocking differences
//
// 2. Memory writes are intercepted for hardware and
// specialized module functions are called to act on the update
// Pros: more event driven, potentially better performance

uint8_t gb_mem_load(uint16_t addr){
  uint8_t val = 0;
  bool mmap_hw = true;
  switch(addr){
    default: {
      mmap_hw = false;
      break;
    }
  }
  if(!mmap_hw){
    // This is a 
  }

  return val;
}

void gb_mem_store(uint16_t addr, uint8_t val){

}