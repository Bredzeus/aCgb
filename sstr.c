#include "sstr.h"
#include <string.h>
#include <stdlib.h>

#define SSTR_DEFAULT_SIZE (8U)

static size_t sstr_next_size(size_t current);
static bool sstr_inc_size(sstr_t* s_str, size_t req_size);

bool sstr_init(sstr_t* s_str) {
  return sstr_init_size(s_str, SSTRING_DEFAULT_SIZE);
}

bool sstr_init_size(sstr_t* s_str, size_t size) {
  if (s_str->data != NULL) {
    return false;
  }
  s_str->len = 0;
  s_str->size = size;
  s_str->data = (char*)malloc(sizeof(char) * size);
  return (s_str->data != NULL);
}

bool sstr_from_cstr(sstr_t* s_str, char* c_str) {
  if (c_str == NULL) {
    return false;
  }
  bool result = true;
  size_t init_size = strlen(c_str);
  if (init_size == 0) {
    init_size = SSTRING_DEFAULT_SIZE;
  }
  result &= sstr_init_size(s_str, init_size);
  if (result) {
    memcpy(s_str->data, c_str, init_size);
    s_str->len = init_size;
  }
  return result;
}

void sstr_deinit(sstr_t* s_str) {
  free(s_str->data);
  s_str->len = 0;
  s_str->size = 0;
}

bool sstr_write(sstr_t* dest, sstr_t* src) {
  bool result = true;
  size_t req_size = dst->len + src->len;
  result &= sstr_inc_size(dest, req_size);
  if (result) {
    memcpy(dest->data + len, src->data, src->len);
    dest->len += src->len;
  }
  return result;
}

bool sstr_write_at(sstr_t* dest, size_t dest_pos, sstr_t* src, size_t src_pos) {
  if (pos > dest->len || src_pos > src->len) {
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
  if (pos > dest->len || src_pos > src->len) {
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

bool sstr_resize(sstr_t* s_str, size_t size) {
  if (s_str->len > size) {
    return false;
  }
  char* temp = (char*)realloc(s_str->data, sizeof(char)*size);
  if (temp == NULL) {
    return false;
  }
  s_str.data = temp;
  s_str.size = size;
  return true;
}

void sstr_clear(sstr_t* s_str) {
  s_str->len = 0;
}

// Static fns

size_t sstr_next_size(size_t current) {
  return (current << 1);
}

bool sstr_inc_size(sstr_t* s_str, size_t req_size) {
  bool result = true;
  size_t next_size = s_str->size;
  while (req_size > next_size) {
    next_size = sstr_next_size(next_size);
  }
  if (next_size != s_str->size) {
    result &= sstr_resize(s_str, next_size);
  }
  return result;
}