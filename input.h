#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>

typedef enum {
  INPUT_METHOD_KEYBOARD = 0,
  NUM_INPUT_METHOD
} input_method_t;

#define INPUT_BUTTON_RIGHT_MASK (1U << 0)
#define INPUT_BUTTON_LEFT_MASK (1U << 1)
#define INPUT_BUTTON_UP_MASK (1U << 2)
#define INPUT_BUTTON_DOWN_MASK (1U << 3)
#define INPUT_BUTTON_A_MASK (1U << 0)
#define INPUT_BUTTON_B_MASK (1U << 1)
#define INPUT_BUTTON_SELECT_MASK (1U << 2)
#define INPUT_BUTTON_START_MASK (1U << 3)
#define INPUT_SELECT_PAD_MASK (1U << 4)
#define INPUT_SELECT_BUTTONS_MASK (1U << 5)

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

uint8_t input_construct_register(input_state_t* inputs, uint8_t io_reg);
uint8_t input_read(input_state_t* inputs);

#endif