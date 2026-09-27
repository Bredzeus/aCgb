#ifndef CBUF_H_
#define CBUF_H_

// Kinda stupid circular buffer implementation
// Abuses void* to handle different datatypes

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct cbuf_s {
  void* buf;
  size_t buf_size; // number of items in buffer
  size_t item_size; // sizeof datatype in buffer
  size_t wr_idx;
  size_t rd_idx;
} cbuf_t;

bool cbuf_init(cbuf_t* cbuf, size_t item_size, size_t buf_size);
void cbuf_deinit(cbuf_t* cbuf);
void cbuf_wr(cbuf_t* cbuf, const void* val);
void* cbuf_rd(cbuf_t* cbuf);
size_t cbuf_size(cbuf_t* cbuf);

#endif // CBUF_H_