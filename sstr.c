#include "sstr.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SSTR_DEFAULT_SIZE (8U)

static size_t sstr_next_size(size_t current);
static bool sstr_inc_size(sstr_t* sstr, size_t req_size);

bool sstr_init(sstr_t* sstr) {
  return sstr_init_size(sstr, SSTR_DEFAULT_SIZE);
}

bool sstr_init_size(sstr_t* sstr, size_t size) {
  if (sstr->data != NULL && sstr->size != 0) {
    return false;
  }
  sstr->len = 0;
  sstr->size = size;
  sstr->data = (char*)malloc(sizeof(char) * size);
  return (sstr->data != NULL);
}

bool sstr_from_cstr(sstr_t* sstr, char* c_str) {
  if (c_str == NULL) {
    return false;
  }
  bool result = true;
  size_t init_size = strlen(c_str);
  if (init_size == 0) {
    init_size = SSTR_DEFAULT_SIZE;
  }
  result &= sstr_init_size(sstr, init_size);
  if (result) {
    memcpy(sstr->data, c_str, init_size);
    sstr->len = init_size;
  }
  return result;
}

void sstr_deinit(sstr_t* sstr) {
  free(sstr->data);
  sstr->len = 0;
  sstr->size = 0;
}

bool sstr_write(sstr_t* dest, sstr_t* src) {
  bool result = true;
  size_t req_size = dest->len + src->len;
  result &= sstr_inc_size(dest, req_size);
  if (result) {
    memcpy(dest->data + dest->len, src->data, src->len);
    dest->len += src->len;
  }
  return result;
}

bool sstr_write_at(sstr_t* dest, size_t dest_pos, sstr_t* src, size_t src_pos) {
  if (dest_pos > dest->len || src_pos > src->len) {
    return false;
  }
  bool result = true;
  size_t cpy_len = src->len - src_pos;
  size_t req_size = dest_pos + cpy_len;
  result &= sstr_inc_size(dest, req_size);
  if (result) {
    memcpy(dest->data + dest_pos, src->data + src_pos, cpy_len);
    if (req_size > dest->len) {
      dest->len = req_size;
    }
  }
  return result;
}

bool sstr_write_n_at(sstr_t* dest, size_t dest_pos, sstr_t* src, size_t src_pos, size_t n) {
  if (dest_pos > dest->len || src_pos > src->len) {
    return false;
  }
  if (src_pos + n > src->len) {
    return false;
  }
  bool result = true;
  size_t req_size = dest_pos + n;
  result &= sstr_inc_size(dest, req_size);
  if (result) {
    memcpy(dest->data + dest_pos, src->data + src_pos, n);
    if (req_size > dest->len) {
      dest->len = req_size;
    }
  }
  return result;
}

bool sstr_resize(sstr_t* sstr, size_t size) {
  if (sstr->len > size) {
    return false;
  }
  char* temp = (char*)realloc(sstr->data, sizeof(char) * size);
  if (temp == NULL) {
    return false;
  }
  sstr->data = temp;
  sstr->size = size;
  return true;
}

void sstr_clear(sstr_t* sstr) {
  sstr->len = 0;
}

void sstr_print(sstr_t* sstr) {
  printf("%.*s", (int)sstr->len, sstr->data);
}

// Static fns

size_t sstr_next_size(size_t current) {
  return (current << 1);
}

bool sstr_inc_size(sstr_t* sstr, size_t req_size) {
  bool result = true;
  size_t next_size = sstr->size;
  while (req_size > next_size) {
    next_size = sstr_next_size(next_size);
  }
  if (next_size != sstr->size) {
    result &= sstr_resize(sstr, next_size);
  }
  return result;
}