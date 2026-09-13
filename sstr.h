#ifndef SSTR_H_
#define SSTR_H_
#include <stdbool.h>

typedef struct sstr_s {
  char* data;
  size_t len; // used space in chars
  size_t size; // allocated space in chars
} sstr_t;

bool sstr_init(sstr_t* s_str);
bool sstr_init_size(sstr_t* s_str, size_t size);
bool sstr_from_cstr(sstr_t* s_str, char* c_str);
void sstr_deinit(sstr_t* s_str);
bool sstr_write(sstr_t* dest, sstr_t* src);
bool sstr_write_at(sstr_t* dest, size_t dest_pos, sstr_t* src, size_t src_pos);
bool sstr_write_n_at(sstr_t* dest, size_t dest_pos, sstr_t* src, size_t src_pos, size_t n);
bool sstr_resize(sstr_t* s_str, size_t size);
void sstr_clear(sstr_t* s_str);

#endif // SSTRING_H_