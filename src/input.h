#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  INPUT_DEVICE_KEYBOARD = 0,
  INPUT_DEVICE_CONTROLLER,
  NUM_INPUT_DEVICE
} input_device_t;

typedef struct input_state_s {
  bool button_right;
  bool button_left;
  bool button_up;
  bool button_down;
  bool button_A;
  bool button_B;
  bool button_select;
  bool button_start;
  bool speedup;
  bool pause;
} input_state_t;

void input_enable_device(input_device_t device, bool enable);
void input_set_device_state(input_device_t device, input_state_t* input);
uint8_t input_load(uint16_t addr);
void input_store(uint16_t addr, uint8_t val);

#endif