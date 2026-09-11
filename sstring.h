#ifndef SSTRING_H_
#define SSTRING_H_
#include <stdbool.h>

typedef struct sstring_s {
  char* data;
  size_t len; // used space in chars
  size_t size; // allocated space in chars
} sstring_t;

bool sstring_init(sstring_t* s_str);
bool sstring_init_size(sstring_t* s_str, size_t size);
void sstring_deinit(sstring_t* s_str);
bool sstring_write(sstring_t* s_str, char* c_str);
bool sstring_write_n(sstring_t* s_str, char* c_str, size_t len);
bool sstring_write_at(sstring_t* s_str, char* c_str, size_t pos);
bool sstring_write_n_at(sstring_t* s_str, char* c_str, size_t len, size_t pos);
bool sstring_resize(sstring_t* s_str, size_t size);

#endif // SSTRING_H_