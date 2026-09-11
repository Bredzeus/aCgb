#include "sstring.h"
#include <string.h>
#include <stdlib.h>

#define SSTRING_DEFAULT_SIZE (8U)

static size_t sstring_increase_size(size_t current);

bool sstring_init(sstring_t* s_str) {
  return sstring_init_size(s_str, SSTRING_DEFAULT_SIZE);
}

bool sstring_init_size(sstring_t* s_str, size_t size) {
  if (s_str->data != NULL) {
    return false;
  }
  s_str->len = 0;
  s_str->size = size;
  s_str->data = (char*)malloc(sizeof(char) * size);
  return (s_str->data != NULL);
}

void sstring_deinit(sstring_t* s_str) {
  free(s_str->data);
}

bool sstring_write(sstring_t* s_str, char* c_str) {
  bool result = true;
  size_t c_str_len = strlen(c_str) + 1;
  size_t req_size = s_str->len + c_str_len;
  size_t next_size = s_str->size;
  while (req_size > next_size) {
    next_size = sstring_next_size(next_size);
  }
  if (next_size != s_str->size) {
    result &= sstring_resize(s_str, next_size);
  }
  if (result) {
    strcpy(s_str->data + len, c_str);
    // Todo: how to handle null terminator
  }

}

bool sstring_write_n(sstring_t* s_str, char* c_str, size_t len) {

}

bool sstring_write_at(sstring_t* s_str, char* c_str, size_t pos) {

}

bool sstring_write_n_at(sstring_t* s_str, char* c_str, size_t len, size_t pos) {

}

bool sstring_resize(sstring_t* s_str, size_t size) {
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

size_t sstring_next_size(size_t current) {
  return (current << 1);
}