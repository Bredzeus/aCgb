#include "common.h"

inline bool check_range_inc(unsigned val, unsigned lower, unsigned upper) {
  return (val >= lower && val <= upper);
}

inline bool check_range_exc(unsigned val, unsigned lower, unsigned upper) {
  return (val > lower && val < upper);
}
