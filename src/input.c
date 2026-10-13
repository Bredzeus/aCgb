#include "input.h"
#include <string.h>
#include "common.h"

#define INPUT_REG_BUTTON_RIGHT (0U)
#define INPUT_REG_BUTTON_LEFT (1U)
#define INPUT_REG_BUTTON_UP (2U)
#define INPUT_REG_BUTTON_DOWN (3U)
#define INPUT_REG_BUTTON_A (0U)
#define INPUT_REG_BUTTON_B (1U)
#define INPUT_REG_BUTTON_SELECT (2U)
#define INPUT_REG_BUTTON_START (3U)
#define INPUT_REG_SELECT_PAD (4U)
#define INPUT_REG_SELECT_BUTTONS (5U)

static uint8_t input_joypad = 0; // 0xFF00

static input_state_t input_devices[NUM_INPUT_DEVICE] = {0};
static bool input_devices_enable[NUM_INPUT_DEVICE] = {0};

static void input_construct_register(void);

void input_enable_device(input_device_t device, bool enable) {
  if (device < NUM_INPUT_DEVICE) {
    input_devices_enable[device] = enable;
  }
}

void input_set_device_state(input_device_t device, input_state_t* input) {
  if (device < NUM_INPUT_DEVICE) {
    memcpy(&input_devices[device], input, sizeof(input_state_t));
  }
}

uint8_t input_load(uint16_t addr) {
  (void)addr;
  input_construct_register();
  return input_joypad;
}

void input_store(uint16_t addr, uint8_t val) {
  (void)addr;
  input_joypad = val;
}

void input_construct_register(void) {
  input_state_t input_or = {0};
  for (input_device_t i = INPUT_DEVICE_KEYBOARD; i < NUM_INPUT_DEVICE; i++) {
    if (!input_devices_enable[i]) {
      continue;
    }
    input_or.button_right |= input_devices[i].button_right;
    input_or.button_left |= input_devices[i].button_left;
    input_or.button_up |= input_devices[i].button_up;
    input_or.button_down |= input_devices[i].button_down;
    input_or.button_A |= input_devices[i].button_A;
    input_or.button_B |= input_devices[i].button_B;
    input_or.button_select |= input_devices[i].button_select;
    input_or.button_start |= input_devices[i].button_start;
    input_or.speedup |= input_devices[i].speedup;
    input_or.pause |= input_devices[i].pause;
  }

  uint8_t read_type = (!GET_BIT(input_joypad, INPUT_REG_SELECT_PAD)) ? INPUT_REG_SELECT_PAD : 0;
  read_type += (!GET_BIT(input_joypad, INPUT_REG_SELECT_BUTTONS)) ? INPUT_REG_SELECT_BUTTONS : 0;
  if (read_type == INPUT_REG_SELECT_PAD) {
    if (input_or.button_right) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_RIGHT);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_RIGHT);
    }
    if (input_or.button_left) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_LEFT);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_LEFT);
    }
    if (input_or.button_up) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_UP);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_UP);
    }
    if (input_or.button_down) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_DOWN);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_DOWN);
    }
  } else if (read_type == INPUT_REG_SELECT_BUTTONS) {
    if (input_or.button_A) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_A);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_A);
    }
    if (input_or.button_B) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_B);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_B);
    }
    if (input_or.button_select) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_SELECT);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_SELECT);
    }
    if (input_or.button_start) {
      CLEAR_BIT(input_joypad, INPUT_REG_BUTTON_START);
    } else {
      SET_BIT(input_joypad, INPUT_REG_BUTTON_START);
    }
  } else {
    input_joypad |= 0x0F; // upper nibble unchanged, lower all 1
  }
}