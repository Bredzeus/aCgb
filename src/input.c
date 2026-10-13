#include "input.h"
#include <string.h>
#include "common.h"
#include "mem_map.h"

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

static mem_interface_t input_mem_io = {0};

static void input_construct_register(void);
static void input_combine_devices(input_state_t* combined);

void input_init(mem_interface_t* mem_io) {
  input_mem_io.load = mem_io->load;
  input_mem_io.store = mem_io->store;
}

void input_enable_device(input_device_t device, bool enable) {
  if (device < NUM_INPUT_DEVICE) {
    input_devices_enable[device] = enable;
  }
}

void input_set_device_state(input_device_t device, input_state_t* input) {
  input_state_t last_state;
  input_combine_devices(&last_state);
  if (device < NUM_INPUT_DEVICE) {
    memcpy(&input_devices[device], input, sizeof(input_state_t));
  }
  input_state_t next_state;
  input_combine_devices(&next_state);
  bool set_interrupt = false;
  uint8_t read_type = (!GET_BIT(input_joypad, INPUT_REG_SELECT_PAD)) ? INPUT_REG_SELECT_PAD : 0;
  read_type += (!GET_BIT(input_joypad, INPUT_REG_SELECT_BUTTONS)) ? INPUT_REG_SELECT_BUTTONS : 0;
  if (read_type == INPUT_REG_SELECT_PAD) {
    if ((last_state.button_right && !next_state.button_right)
        || (last_state.button_left && !next_state.button_left)
        || (last_state.button_up && !next_state.button_up)
        || (last_state.button_down && !next_state.button_down)) {
      set_interrupt = true;
    }
  } else if (read_type == INPUT_REG_SELECT_BUTTONS) {
    if ((last_state.button_A && !next_state.button_A)
        || (last_state.button_B && !next_state.button_B)
        || (last_state.button_select && !next_state.button_select)
        || (last_state.button_start && !next_state.button_start)) {
      set_interrupt = true;
    }
  }
  if (set_interrupt) {
    uint8_t int_flags = input_mem_io.load(ADDR_IF);
    SET_BIT(int_flags, INT_JOYPAD);
    input_mem_io.store(ADDR_IF, int_flags);
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
  input_combine_devices(&input_or);
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

void input_combine_devices(input_state_t* combined) {
  for (input_device_t i = INPUT_DEVICE_KEYBOARD; i < NUM_INPUT_DEVICE; i++) {
    if (!input_devices_enable[i]) {
      continue;
    }
    combined->button_right |= input_devices[i].button_right;
    combined->button_left |= input_devices[i].button_left;
    combined->button_up |= input_devices[i].button_up;
    combined->button_down |= input_devices[i].button_down;
    combined->button_A |= input_devices[i].button_A;
    combined->button_B |= input_devices[i].button_B;
    combined->button_select |= input_devices[i].button_select;
    combined->button_start |= input_devices[i].button_start;
    combined->speedup |= input_devices[i].speedup;
    combined->pause |= input_devices[i].pause;
  }
}