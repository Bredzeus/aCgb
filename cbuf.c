#include "cbuf.h"
#include <stdlib.h>
#include <string.h>

bool cbuf_init(cbuf_t* cbuf, size_t item_size, size_t buf_size) {
  if (cbuf->buf != NULL) {
    return false;
  }
  if (item_size == 0 || buf_size == 0) {
    return false;
  }
  cbuf->buf = malloc(item_size * buf_size);
  if (cbuf->buf == NULL) {
    return false;
  }
  cbuf->item_size = item_size;
  cbuf->buf_size = buf_size;
  cbuf->wr_idx = 0;
  cbuf->rd_idx = 0;
  return true;
}

void cbuf_deinit(cbuf_t* cbuf) {
  free(cbuf->buf);
}

void cbuf_write(cbuf_t* cbuf, const void* val) {
  memcpy((cbuf->buf + cbuf->wr_idx), val, cbuf->item_size);
  cbuf->wr_idx = (cbuf->wr_idx + 1) % cbuf->buf_size;
}

void* cbuf_read(cbuf_t* cbuf) {
  void* retval = cbuf->buf + cbuf->rd_idx;
  cbuf->rd_idx = (cbuf->rd_idx + 1) % cbuf->buf_size;
  return retval;
}

size_t cbuf_size(cbuf_t* cbuf) {
  return ((cbuf->buf_size + cbuf->wr_idx - cbuf->rd_idx) % cbuf->buf_size);
}

void cbuf_clear(cbuf_t* cbuf) {
  cbuf->rd_idx = cbuf->wr_idx;
}