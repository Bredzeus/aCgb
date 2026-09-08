#include <stdio.h>

#include "common.h"
#include "sm83.h"
#include "mem.h"


int main() {
  sm83_t sm83;
  sm83_init(&sm83, mem_load, mem_store);
  sm83_reset(&sm83);

  #define TEST_BUF_SIZE 250
  char test_buf[TEST_BUF_SIZE];
  sstring_t test_str;
  test_str.buf = test_buf;
  test_str.len = 0;
  test_str.size = TEST_BUF_SIZE;

  sm83_dump(&sm83, &test_str);
  printf("%s\n", test_str.buf);

  return 0;
}