#include "input.h"


uint8_t input_construct_register(input_state_t* inputs, uint8_t io_reg){
  uint8_t out = io_reg;
  uint8_t read_type = (~io_reg) & (INPUT_SELECT_PAD_MASK | INPUT_SELECT_BUTTONS_MASK);
  switch (read_type) {
    case INPUT_SELECT_PAD_MASK:
      out = inputs->button_right ? (out & (~INPUT_BUTTON_RIGHT_MASK)) : (out | INPUT_BUTTON_RIGHT_MASK);
      out = inputs->button_left ? (out & (~INPUT_BUTTON_LEFT_MASK)) : (out | INPUT_BUTTON_LEFT_MASK);
      out = inputs->button_up ? (out & (~INPUT_BUTTON_UP_MASK)) : (out | INPUT_BUTTON_UP_MASK);
      out = inputs->button_down ? (out & (~INPUT_BUTTON_DOWN_MASK)) : (out | INPUT_BUTTON_DOWN_MASK);
      break;
    case INPUT_SELECT_BUTTONS_MASK:
      out = inputs->button_A ? (out & (~INPUT_BUTTON_A_MASK)) : (out | INPUT_BUTTON_A_MASK);
      out = inputs->button_B ? (out & (~INPUT_BUTTON_B_MASK)) : (out | INPUT_BUTTON_B_MASK);
      out = inputs->button_select ? (out & (~INPUT_BUTTON_SELECT_MASK)) : (out | INPUT_BUTTON_SELECT_MASK);
      out = inputs->button_start ? (out & (~INPUT_BUTTON_START_MASK)) : (out | INPUT_BUTTON_START_MASK);
      break;
    default:
      out |= 0x0F; // upper nibble unchanged, lower all 1
      break;
  }
  return out;
}