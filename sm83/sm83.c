#include "sm83.h"

#include <stdio.h>
#include "sm83_common.h"
#include "sm83_alu.h"


#define SM83_MID_INDEX_MASK (0b00111000U)
#define SM83_END_INDEX_MASK (0b00000111U)

// Masks for load instruction
#define SM83_LD_MASK (0b11000000U)
#define SM83_LD_PREFIX (0b01000000U)
#define SM83_LD_IMM_PREFIX (0b00000000U)

// Masks for add instruction
#define SM83_ARITH_MASK (0b11111000U)
#define SM83_ADD_PREFIX (0b10000000U)
#define SM83_ADC_PREFIX (0b10001000U)
#define SM83_SUB_PREFIX (0b10010000U)
#define SM83_SBC_PREFIX (0b10011000U)
#define SM83_CP_PREFIX (0b10111000U)
#define SM83_AND_PREFIX (0b10100000U)
#define SM83_OR_PREFIX (0b10110000U)
#define SM83_XOR_PREFIX (0b10101000U)

#define SM83_INC_DEC_MASK (0b11000111U)
#define SM83_INC_PREFIX (0b00000100U)
#define SM83_DEC_PREFIX (0b00000101U)

#define SM83_CB_MASK (0b11111000U)
#define SM83_RLC_PREFIX (0b00000000U)
#define SM83_RRC_PREFIX (0b00001000U)
#define SM83_RL_PREFIX (0b00010000U)
#define SM83_RR_PREFIX (0b00011000U)
#define SM83_SLA_PREFIX (0b00100000U)
#define SM83_SRA_PREFIX (0b00101000U)
#define SM83_SWAP_PREFIX (0b00110000U)
#define SM83_SRL_PREFIX (0b00111000U)

#define SM83_BIT_MASK (0b11000000U)
#define SM83_BIT_PREFIX (0b01000000U)
#define SM83_RES_PREFIX (0b10000000U)
#define SM83_SET_PREFIX (0b11000000U)

typedef enum {
  SM83_INT_RES_NOP,
  SM83_INT_RES_WAKE,
  SM83_INT_RES_EXEC,
  NUM_SM83_INT_RES
} sm83_int_res_t;

static void handle_halt(sm83_t* sm83);
static void handle_stop(sm83_t* sm83);
static void handle_run(sm83_t* sm83);
static sm83_int_res_t handle_interrupt(sm83_t* sm83);

static void exec_cb_op(sm83_t* sm83, uint8_t cb_op);

static void load_reg_std(sm83_t* sm83, uint8_t instr);
static uint8_t reg_lookup_val(sm83_t* sm83, uint8_t index);
static uint8_t* reg_lookup_ptr(sm83_t* sm83, uint8_t index);
static void load_reg_imm(sm83_t* sm83, uint8_t instr);

static void push_rr(sm83_t* sm83, uint8_t instr);
static void pop_rr(sm83_t* sm83, uint8_t instr);

static inline uint16_t combine(uint8_t high, uint8_t low);
static inline void separate(uint16_t comb, uint8_t* high, uint8_t* low);

static inline void jump_relative(sm83_t* sm83, int8_t e);
static inline void call(sm83_t* sm83, uint16_t addr);
static inline void ret(sm83_t* sm83);
static inline uint16_t load_nn(sm83_t* sm83);

void sm83_init(sm83_t* sm83, mem_load_f_t load, mem_store_f_t store){
  // TODO: warn on null
  sm83->io.mem_load = load;
  sm83->io.mem_store = store;
}

void sm83_reset(sm83_t* sm83) {
  // TODO implement version dependent boot rom
  // For now, init is based on AGB
  sm83->reg_file.pc = 0x0100;
  sm83->reg_file.sp = 0xfffe;
  sm83->reg_file.a = 0x11;
  sm83->reg_file.f = 0x00;
  sm83->reg_file.b = 0x01;
  sm83->reg_file.c = 0x00;
  sm83->reg_file.d = 0xff;
  sm83->reg_file.e = 0x56;
  sm83->reg_file.h = 0x00;
  sm83->reg_file.l = 0x0d;
  sm83->reg_file.ir = 0;
  sm83->reg_file.ie = 0;

  sm83->state = SM83_STATE_RUN;
  sm83->ime = false;
}

uint32_t sm83_run(sm83_t* sm83) {
  sm83->cycles = 0;
  switch(sm83->state) {
  case SM83_STATE_RUN:
    handle_run(sm83);
    break;
  case SM83_STATE_HALT:
    handle_halt(sm83);
    break;
  case SM83_STATE_STOP:
    handle_stop(sm83);
    break;
  default:
    break;
  }
  return sm83->cycles;
}

void sm83_dump(sm83_t* sm83, sstring_t* str) {
  const char print_str[] = "SM83 register file:\npc: 0x%04x\nsp: 0x%04x\na:  0x%02x\nf:  0x%02x\nb:  0x%02x\nc:  0x%02x\nd:  0x%02x\ne:  0x%02x\nh:  0x%02x\nl:  0x%02x\nir: 0x%02x\nie: 0x%02x\n";
  if (str->size < sizeof(print_str)) {
    return;
  }
  str->len = snprintf(str->buf, str->size, print_str, 
    sm83->reg_file.pc,
    sm83->reg_file.sp,
    sm83->reg_file.a,
    sm83->reg_file.f,
    sm83->reg_file.b,
    sm83->reg_file.c,
    sm83->reg_file.d,
    sm83->reg_file.e,
    sm83->reg_file.h,
    sm83->reg_file.l,
    sm83->reg_file.ir,
    sm83->reg_file.ie
  );
}

void handle_halt(sm83_t* sm83) {
  if(handle_interrupt(sm83) != SM83_INT_RES_NOP){
    sm83->state = SM83_STATE_RUN;
  }
}

void handle_stop(sm83_t* sm83) {
  // exits stop on controller input
  uint8_t joypad = sm83->io.mem_load(ADDR_JOYPAD);
  if((joypad & 0x0F) != 0x0F){
    sm83->state = SM83_STATE_RUN;
  }
}

void handle_run(sm83_t* sm83) {
  if(handle_interrupt(sm83) == SM83_INT_RES_EXEC){
    return; // skip everything else if an interrupt is serviced
  }

  const uint8_t instr = sm83->io.mem_load(sm83->reg_file.pc);
  sm83->reg_file.pc++;

  switch(instr) {
  // Handle special cases
  case SM83_INSTR_NOP:
    sm83->cycles = 1;
    break;
  case SM83_INSTR_HALT:
    sm83->state = SM83_STATE_HALT;
    sm83->cycles = 1; // Assuming it takes a cycle to halt, not that it really matters
    break;
  case SM83_INSTR_STOP:
    sm83->state = SM83_STATE_STOP;
    // TODO: theres some jank here to implement
    sm83->cycles = 1; // Assumption
    break;
  case SM83_INSTR_EI:
    sm83->ime = true;
    sm83->cycles = 1;
    break;
  case SM83_INSTR_DI:
    sm83->ime = false;
    sm83->cycles = 1;
    break;
  case SM83_INSTR_CB_op: {
    const uint8_t cb_op = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    exec_cb_op(sm83, cb_op);
    break;
  }
  case SM83_INSTR_LD_A_pBC: {
    const uint16_t addr = combine(sm83->reg_file.b, sm83->reg_file.c);
    sm83->reg_file.a = sm83->io.mem_load(addr);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_A_pDE: {
    const uint16_t addr = combine(sm83->reg_file.d, sm83->reg_file.e);
    sm83->reg_file.a = sm83->io.mem_load(addr);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_pBC_A: {
    const uint16_t addr = combine(sm83->reg_file.b, sm83->reg_file.c);
    sm83->io.mem_store(addr, sm83->reg_file.a);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_pDE_A: {
    const uint16_t addr = combine(sm83->reg_file.d, sm83->reg_file.e);
    sm83->io.mem_store(addr, sm83->reg_file.a);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_A_pnn: {
    const uint16_t nn = load_nn(sm83);
    sm83->reg_file.a = sm83->io.mem_load(nn);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_LD_pnn_A: {
    const uint16_t nn = load_nn(sm83);
    sm83->io.mem_store(nn, sm83->reg_file.a);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_LDH_A_pC: {
    sm83->reg_file.a = sm83->io.mem_load(combine(0xffU, sm83->reg_file.c));
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LDH_pC_A: {
    const uint16_t addr = combine(0xffU, sm83->reg_file.c);
    sm83->io.mem_store(addr, sm83->reg_file.a);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LDH_A_pn: {
    const uint8_t low = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->reg_file.a = sm83->io.mem_load(combine(0xffU, low));
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_LDH_pn_A: {
    const uint8_t low = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    const uint16_t addr = combine(0xffU, low);
    sm83->io.mem_store(addr, sm83->reg_file.a);
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_LD_A_pHLm: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->reg_file.a = sm83->io.mem_load(hl);
    hl--;
    separate(hl, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_pHLm_A: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->io.mem_store(hl, sm83->reg_file.a);
    hl--;
    separate(hl, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_A_pHLp: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->reg_file.a = sm83->io.mem_load(hl);
    hl++;
    separate(hl, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_pHLp_A: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->io.mem_store(hl, sm83->reg_file.a);
    hl++;
    separate(hl, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_LD_BC_nn: {
    sm83->reg_file.c = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->reg_file.b = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_LD_DE_nn: {
    sm83->reg_file.e = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->reg_file.d = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_LD_HL_nn: {
    sm83->reg_file.l = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->reg_file.h = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_LD_SP_nn: {
    const uint16_t nn = load_nn(sm83);
    sm83->reg_file.sp = nn;
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_LD_pnn_SP: {
    uint16_t nn = load_nn(sm83);
    uint8_t sp_l = 0;
    uint8_t sp_h = 0;
    separate(sm83->reg_file.sp, &sp_h, &sp_l);
    sm83->io.mem_store(nn, sp_l);
    nn++;
    sm83->io.mem_store(nn, sp_h);
    sm83->cycles += 5;
    break;
  }
  case SM83_INSTR_LD_SP_HL: {
    const uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->reg_file.sp = hl;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_PUSH_BC:
  case SM83_INSTR_PUSH_DE:
  case SM83_INSTR_PUSH_HL:
  case SM83_INSTR_PUSH_AF: {
    push_rr(sm83, instr);
    break;
  }
  case SM83_INSTR_POP_BC:
  case SM83_INSTR_POP_DE:
  case SM83_INSTR_POP_HL:
  case SM83_INSTR_POP_AF: {
    pop_rr(sm83, instr);
    break;
  }
  case SM83_INSTR_LD_HL_SPpe: {
    int8_t e = (int8_t)sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    uint16_t result = 0;
    if(e < 0){
      e *= -1;
      result = sm83->reg_file.sp - ((uint16_t)e);
    }else{
      result = sm83->reg_file.sp + ((uint16_t)e);
    }
    separate(result, &sm83->reg_file.h, &sm83->reg_file.l);
    const uint16_t carry = sm83->reg_file.sp & ((uint16_t)e);
    sm83->reg_file.f &= ~FLAG_ZERO;
    sm83->reg_file.f &= ~FLAG_SUB;
    if(carry & (1U << 3)){
      sm83->reg_file.f |= FLAG_HALF_CARRY;
    }else{
      sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    }
    if(carry & (1U << 7)){
      sm83->reg_file.f |= FLAG_CARRY;
    }else{
      sm83->reg_file.f &= ~FLAG_CARRY;
    }
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_ADD_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_ADD, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_ADC_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_ADC, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_SUB_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_SUB, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_SBC_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_SBC, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_CP_n: {
    // Same as SUB n but we don't write back to the accumulator
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_SUB, &alu);
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_AND_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_AND, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_OR_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_OR, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_XOR_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = n,
      .flags = sm83->reg_file.f
    };
    sm83_alu_execute(SM83_ALU_OP_XOR, &alu);
    sm83->reg_file.a = alu.primary;
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_CCF: {
    if (sm83->reg_file.f & FLAG_CARRY) {
      sm83->reg_file.f &= (~FLAG_CARRY);
    } else {
      sm83->reg_file.f |= (FLAG_CARRY);
    }
    sm83->reg_file.f &= (~FLAG_SUB);
    sm83->reg_file.f &= (~FLAG_HALF_CARRY);
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_SCF: {
    sm83->reg_file.f |= (FLAG_CARRY);
    sm83->reg_file.f &= (~FLAG_SUB);
    sm83->reg_file.f &= (~FLAG_HALF_CARRY);
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_DAA: {
    uint8_t adjust = 0;
    sm83_alu_t alu = {
      .primary = sm83->reg_file.a,
      .secondary = 0,
      .flags = sm83->reg_file.f
    };
    if (sm83->reg_file.f & FLAG_SUB) {
      if (sm83->reg_file.f & FLAG_HALF_CARRY) {
        adjust += 0x6U;
      }
      if (sm83->reg_file.f & FLAG_CARRY) {
        adjust += 0x60U;
      }
      alu.secondary = adjust;
      sm83_alu_execute(SM83_ALU_OP_SUB, &alu);
      sm83->reg_file.a = alu.primary;
      sm83->reg_file.f = alu.flags;
    } else {
      if (sm83->reg_file.f & FLAG_HALF_CARRY || sm83->reg_file.a > 0x9U) {
        adjust += 0x6U;
      }
      if (sm83->reg_file.f & FLAG_CARRY || sm83->reg_file.a > 0x99U) {
        adjust += 0x60U;
        sm83->reg_file.f |= FLAG_CARRY;
      }
      alu.secondary = adjust;
      sm83_alu_execute(SM83_ALU_OP_ADD, &alu);
      sm83->reg_file.a = alu.primary;
      sm83->reg_file.f = alu.flags;
    }
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_CPL: {
    sm83->reg_file.a = ~sm83->reg_file.a;
    sm83->reg_file.f |= FLAG_SUB;
    sm83->reg_file.f |= FLAG_HALF_CARRY;
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_INC_BC: {
    uint16_t bc = combine(sm83->reg_file.b, sm83->reg_file.c);
    bc++;
    separate(bc, &sm83->reg_file.b, &sm83->reg_file.c);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_INC_DE: {
    uint16_t de = combine(sm83->reg_file.d, sm83->reg_file.e);
    de++;
    separate(de, &sm83->reg_file.d, &sm83->reg_file.e);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_INC_HL: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    hl++;
    separate(hl, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_INC_SP: {
    sm83->reg_file.sp++;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_ADD_HL_BC: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    uint16_t bc = combine(sm83->reg_file.b, sm83->reg_file.c);
    sm83_alu16_t alu = {
      .primary = hl,
      .secondary = bc,
      .flags = sm83->reg_file.f
    };
    sm83_alu16_execute(SM83_ALU_OP_ADD, &alu);
    separate(alu.primary, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_ADD_HL_DE: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    uint16_t de = combine(sm83->reg_file.d, sm83->reg_file.e);
    sm83_alu16_t alu = {
      .primary = hl,
      .secondary = de,
      .flags = sm83->reg_file.f
    };
    sm83_alu16_execute(SM83_ALU_OP_ADD, &alu);
    separate(alu.primary, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_ADD_HL_HL: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83_alu16_t alu = {
      .primary = hl,
      .secondary = hl,
      .flags = sm83->reg_file.f
    };
    sm83_alu16_execute(SM83_ALU_OP_ADD, &alu);
    separate(alu.primary, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_ADD_HL_SP: {
    uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83_alu16_t alu = {
      .primary = hl,
      .secondary = sm83->reg_file.sp,
      .flags = sm83->reg_file.f
    };
    sm83_alu16_execute(SM83_ALU_OP_ADD, &alu);
    separate(alu.primary, &sm83->reg_file.h, &sm83->reg_file.l);
    sm83->reg_file.f = alu.flags;
    sm83->cycles += 2;
    break;
  }
  case SM83_INSTR_ADD_SP_n: {
    const uint8_t n = sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    uint16_t carry = sm83->reg_file.sp & ((uint16_t)n);
    sm83->reg_file.sp += ((uint16_t)n);
    sm83->reg_file.f &= ~FLAG_ZERO;
    sm83->reg_file.f &= ~FLAG_SUB;
    if (carry & MASK_CARRY) {
      sm83->reg_file.f |= FLAG_CARRY;
    } else {
      sm83->reg_file.f &= ~FLAG_CARRY;
    }
    if (carry & MASK_HALF_CARRY) {
      sm83->reg_file.f |= FLAG_HALF_CARRY;
    } else {
      sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    }
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RLCA: {
    const uint8_t bit7 = (sm83->reg_file.a & (1U << 7)) ? 1U : 0U;
    sm83->reg_file.a = (sm83->reg_file.a << 1);
    if(bit7) {
      sm83->reg_file.a |= 1U;
      sm83->reg_file.f |= FLAG_CARRY;
    }else{
      sm83->reg_file.f &= ~FLAG_CARRY;
    }
    sm83->reg_file.f &= ~FLAG_ZERO;
    sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    sm83->reg_file.f &= ~FLAG_SUB;
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_RRCA: {
    const uint8_t bit0 = sm83->reg_file.a & 1U;
    sm83->reg_file.a = (sm83->reg_file.a >> 1);
    if(bit0) {
      sm83->reg_file.a |= (1U << 7);
      sm83->reg_file.f |= FLAG_CARRY;
    }else{
      sm83->reg_file.f &= ~FLAG_CARRY;
    }
    sm83->reg_file.f &= ~FLAG_ZERO;
    sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    sm83->reg_file.f &= ~FLAG_SUB;
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_RLA: {
    const uint8_t bit7 = (sm83->reg_file.a & (1U << 7)) ? 1U : 0U;
    sm83->reg_file.a = (sm83->reg_file.a << 1);
    if(sm83->reg_file.f & FLAG_CARRY){
      sm83->reg_file.a |= 1U;
    }
    if(bit7) {
      sm83->reg_file.f |= FLAG_CARRY;
    }else{
      sm83->reg_file.f &= ~FLAG_CARRY;
    }
    sm83->reg_file.f &= ~FLAG_ZERO;
    sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    sm83->reg_file.f &= ~FLAG_SUB;
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_RRA: {
    const uint8_t bit0 = sm83->reg_file.a & 1U;
    sm83->reg_file.a = (sm83->reg_file.a >> 1);
    if(sm83->reg_file.f & FLAG_CARRY){
      sm83->reg_file.a |= (1U << 7);
    }
    if(bit0) {
      sm83->reg_file.f |= FLAG_CARRY;
    }else{
      sm83->reg_file.f &= ~FLAG_CARRY;
    }
    sm83->reg_file.f &= ~FLAG_ZERO;
    sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    sm83->reg_file.f &= ~FLAG_SUB;
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_JP_nn: {
    const uint16_t nn = load_nn(sm83);
    sm83->reg_file.pc = nn;
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_JP_HL: {
    sm83->reg_file.pc = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->cycles++;
    break;
  }
  case SM83_INSTR_JP_NZ_nn: {
    const uint16_t nn = load_nn(sm83);
    if(!(sm83->reg_file.f & FLAG_ZERO)){
      sm83->reg_file.pc = nn;
      sm83->cycles += 4;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_JP_Z_nn: {
    const uint16_t nn = load_nn(sm83);
    if(sm83->reg_file.f & FLAG_ZERO){
      sm83->reg_file.pc = nn;
      sm83->cycles += 4;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_JP_NC_nn: {
    const uint16_t nn = load_nn(sm83);
    if(!(sm83->reg_file.f & FLAG_CARRY)){
      sm83->reg_file.pc = nn;
      sm83->cycles += 4;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_JP_C_nn: {
    const uint16_t nn = load_nn(sm83);
    if(sm83->reg_file.f & FLAG_CARRY){
      sm83->reg_file.pc = nn;
      sm83->cycles += 4;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_JR_e: {
    const int8_t e = (int8_t)sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    jump_relative(sm83, e);
    sm83->cycles += 3;
    break;
  }
  case SM83_INSTR_JR_NZ_e: {
    const int8_t e = (int8_t)sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    if(!(sm83->reg_file.f & FLAG_ZERO)){
      jump_relative(sm83, e);
      sm83->cycles += 3;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_JR_Z_e: {
    const int8_t e = (int8_t)sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    if(sm83->reg_file.f & FLAG_ZERO){
      jump_relative(sm83, e);
      sm83->cycles += 3;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_JR_NC_e: {
    const int8_t e = (int8_t)sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    if(!(sm83->reg_file.f & FLAG_CARRY)){
      jump_relative(sm83, e);
      sm83->cycles += 3;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_JR_C_e: {
    const int8_t e = (int8_t)sm83->io.mem_load(sm83->reg_file.pc);
    sm83->reg_file.pc++;
    if(sm83->reg_file.f & FLAG_CARRY){
      jump_relative(sm83, e);
      sm83->cycles += 3;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_CALL_nn: {
    const uint16_t nn = load_nn(sm83);
    call(sm83, nn);
    sm83->cycles += 6;
    break;
  }
  case SM83_INSTR_CALL_NZ_nn: {
    const uint16_t nn = load_nn(sm83);
    if(!(sm83->reg_file.f & FLAG_ZERO)){
      call(sm83, nn);
      sm83->cycles += 6;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_CALL_Z_nn: {
    const uint16_t nn = load_nn(sm83);
    if(sm83->reg_file.f & FLAG_ZERO){
      call(sm83, nn);
      sm83->cycles += 6;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_CALL_NC_nn: {
    const uint16_t nn = load_nn(sm83);
    if(!(sm83->reg_file.f & FLAG_CARRY)){
      call(sm83, nn);
      sm83->cycles += 6;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_CALL_C_nn: {
    const uint16_t nn = load_nn(sm83);
    if(sm83->reg_file.f & FLAG_CARRY){
      call(sm83, nn);
      sm83->cycles += 6;
    }else{
      sm83->cycles += 3;
    }
    break;
  }
  case SM83_INSTR_RET: {
    ret(sm83);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RET_NZ: {
    if(!(sm83->reg_file.f & FLAG_ZERO)){
      ret(sm83);
      sm83->cycles += 5;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_RET_Z: {
    if(sm83->reg_file.f & FLAG_ZERO){
      ret(sm83);
      sm83->cycles += 5;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_RET_NC: {
    if(!(sm83->reg_file.f & FLAG_CARRY)){
      ret(sm83);
      sm83->cycles += 5;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_RET_C: {
    if(sm83->reg_file.f & FLAG_CARRY){
      ret(sm83);
      sm83->cycles += 5;
    }else{
      sm83->cycles += 2;
    }
    break;
  }
  case SM83_INSTR_RETI: {
    sm83->ime = true;
    ret(sm83);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x00: {
    call(sm83, 0x0000U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x08: {
    call(sm83, 0x0008U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x10: {
    call(sm83, 0x0010U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x18: {
    call(sm83, 0x0018U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x20: {
    call(sm83, 0x0020U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x28: {
    call(sm83, 0x0028U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x30: {
    call(sm83, 0x0030U);
    sm83->cycles += 4;
    break;
  }
  case SM83_INSTR_RST_0x38: {
    call(sm83, 0x0038U);
    sm83->cycles += 4;
    break;
  }
  default: {
    // Handle general cases
    const uint8_t ld_masked_instr = instr & SM83_LD_MASK;
    const uint8_t arith_masked_instr = instr & SM83_ARITH_MASK;
    const uint8_t inc_dec_masked_instr = instr & SM83_INC_DEC_MASK;
    if (ld_masked_instr == SM83_LD_PREFIX) { // Load register
      load_reg_std(sm83, instr);
    } else if (ld_masked_instr == SM83_LD_IMM_PREFIX) { // Load immediate 
      load_reg_imm(sm83, instr);
    } else if (arith_masked_instr == SM83_ADD_PREFIX) { // add register
      const uint8_t val = reg_lookup_val(sm83, instr & SM83_END_INDEX_MASK);
      sm83_alu_t alu = {
        .primary = sm83->reg_file.a,
        .secondary = val,
        .flags = sm83->reg_file.f
      };
      sm83_alu_execute(SM83_ALU_OP_ADD, &alu);
      sm83->reg_file.a = alu.primary;
      sm83->reg_file.f = alu.flags;
      sm83->cycles++;
    } else if (arith_masked_instr == SM83_ADC_PREFIX) { // add with carry register
      const uint8_t val = reg_lookup_val(sm83, instr & SM83_END_INDEX_MASK);
      sm83_alu_t alu = {
        .primary = sm83->reg_file.a,
        .secondary = val,
        .flags = sm83->reg_file.f
      };
      sm83_alu_execute(SM83_ALU_OP_ADC, &alu);
      sm83->reg_file.a = alu.primary;
      sm83->reg_file.f = alu.flags;
      sm83->cycles++;
    } else if (arith_masked_instr == SM83_SUB_PREFIX) { // sub register
      const uint8_t val = reg_lookup_val(sm83, instr & SM83_END_INDEX_MASK);
      sm83_alu_t alu = {
        .primary = sm83->reg_file.a,
        .secondary = val,
        .flags = sm83->reg_file.f
      };
      sm83_alu_execute(SM83_ALU_OP_ADC, &alu);
      sm83->reg_file.a = alu.primary;
      sm83->reg_file.f = alu.flags;
      sm83->cycles++;
    } else if (arith_masked_instr == SM83_SBC_PREFIX) { // sub with carry register
      const uint8_t val = reg_lookup_val(sm83, instr & SM83_END_INDEX_MASK);
      sm83_alu_t alu = {
        .primary = sm83->reg_file.a,
        .secondary = val,
        .flags = sm83->reg_file.f
      };
      sm83_alu_execute(SM83_ALU_OP_ADC, &alu);
      sm83->reg_file.a = alu.primary;
      sm83->reg_file.f = alu.flags;
      sm83->cycles++;
    } else if (arith_masked_instr == SM83_CP_PREFIX) { // compare with register
      const uint8_t val = reg_lookup_val(sm83, instr & SM83_END_INDEX_MASK);
      sm83_alu_t alu = {
        .primary = sm83->reg_file.a,
        .secondary = val,
        .flags = sm83->reg_file.f
      };
      sm83_alu_execute(SM83_ALU_OP_SUB, &alu);
      sm83->reg_file.f = alu.flags;
      sm83->cycles++;
    } else if (inc_dec_masked_instr == SM83_INC_PREFIX) { // increment register
      const uint8_t idx = (instr & SM83_MID_INDEX_MASK) >> 3;
      if (idx == 6) {
        // indirect HL
        const uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
        sm83_alu_t alu = {
          .primary = sm83->io.mem_load(hl),
          .secondary = 0, // unused for INC
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_INC, &alu);
        sm83->io.mem_store(hl, alu.primary);
        sm83->reg_file.f = alu.flags;
        sm83->cycles += 3;
      } else {
        uint8_t* reg_p = reg_lookup_ptr(sm83, idx);
        sm83_alu_t alu = {
          .primary = *reg_p,
          .secondary = 0, // unused for INC
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_INC, &alu);
        *reg_p = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles++;
      }
    } else if (inc_dec_masked_instr == SM83_DEC_PREFIX) { // decrement register
      const uint8_t idx = (instr & SM83_MID_INDEX_MASK) >> 3;
      if (idx == 6) {
        // indirect HL
        const uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
        sm83_alu_t alu = {
          .primary = sm83->io.mem_load(hl),
          .secondary = 0, // unused for DEC
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_DEC, &alu);
        sm83->io.mem_store(hl, alu.primary);
        sm83->reg_file.f = alu.flags;
        sm83->cycles += 3;
      } else {
        uint8_t* reg_p = reg_lookup_ptr(sm83, idx);
        sm83_alu_t alu = {
          .primary = *reg_p,
          .secondary = 0, // unused for DEC
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_DEC, &alu);
        *reg_p = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles++;
      }
    } else if (arith_masked_instr == SM83_AND_PREFIX) {
      const uint8_t idx = (instr & SM83_END_INDEX_MASK);
      if (idx == 6) {
        // indirect HL
        const uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
        sm83_alu_t alu = {
          .primary = sm83->reg_file.a,
          .secondary = sm83->io.mem_load(hl), 
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_AND, &alu);
        sm83->reg_file.a = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles += 2;
      } else {
        uint8_t val = reg_lookup_val(sm83, idx);
        sm83_alu_t alu = {
          .primary = sm83->reg_file.a,
          .secondary = val,
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_AND, &alu);
        sm83->reg_file.a = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles++;
      }
    } else if (arith_masked_instr == SM83_OR_PREFIX) {
      const uint8_t idx = (instr & SM83_END_INDEX_MASK);
      if (idx == 6) {
        // indirect HL
        const uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
        sm83_alu_t alu = {
          .primary = sm83->reg_file.a,
          .secondary = sm83->io.mem_load(hl), 
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_OR, &alu);
        sm83->reg_file.a = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles += 2;
      } else {
        uint8_t val = reg_lookup_val(sm83, idx);
        sm83_alu_t alu = {
          .primary = sm83->reg_file.a,
          .secondary = val,
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_OR, &alu);
        sm83->reg_file.a = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles++;
      }
    } else if (arith_masked_instr == SM83_XOR_PREFIX) {
      const uint8_t idx = (instr & SM83_END_INDEX_MASK);
      if (idx == 6) {
        // indirect HL
        const uint16_t hl = combine(sm83->reg_file.h, sm83->reg_file.l);
        sm83_alu_t alu = {
          .primary = sm83->reg_file.a,
          .secondary = sm83->io.mem_load(hl), 
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_XOR, &alu);
        sm83->reg_file.a = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles += 2;
      } else {
        uint8_t val = reg_lookup_val(sm83, idx);
        sm83_alu_t alu = {
          .primary = sm83->reg_file.a,
          .secondary = val,
          .flags = sm83->reg_file.f
        };
        sm83_alu_execute(SM83_ALU_OP_XOR, &alu);
        sm83->reg_file.a = alu.primary;
        sm83->reg_file.f = alu.flags;
        sm83->cycles++;
      }
    }
    break;
  }
  }
}

void load_reg_std(sm83_t* sm83, uint8_t instr) {
  const uint8_t target_idx = (SM83_MID_INDEX_MASK & instr) >> 3;
  const uint8_t source_idx = SM83_END_INDEX_MASK & instr;
  uint8_t source_val = reg_lookup_val(sm83, source_idx);
  // target_idx 6 is a write to memory
  if (target_idx == 6) {
    uint16_t addr = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->io.mem_store(addr, source_val);
    sm83->cycles++; // Extra cycle for mem access
  } else {
    uint8_t* target = reg_lookup_ptr(sm83, target_idx);
    *target = source_val;
  }
  sm83->cycles++;
}

uint8_t reg_lookup_val(sm83_t* sm83, uint8_t index) {
  uint8_t value = 0;
  switch (index) {
  case 0:
    value = sm83->reg_file.b;
    break;
  case 1:
    value = sm83->reg_file.c;
    break;
  case 2:
    value = sm83->reg_file.d;
    break;
  case 3:
    value = sm83->reg_file.e;
    break;
  case 4:
    value = sm83->reg_file.h;
    break;
  case 5:
    value = sm83->reg_file.l;
    break;
  case 6:
    uint16_t addr = combine(sm83->reg_file.h, sm83->reg_file.l);
    value = sm83->io.mem_load(addr);
    sm83->cycles++; // Extra cycle for mem access
    break;
  case 7:
    value = sm83->reg_file.a;
    break;
  default:
    break;
  }
  return value;
}

uint8_t* reg_lookup_ptr(sm83_t* sm83, uint8_t index) {
  uint8_t* ptr = NULL;
  switch (index) {
  case 0:
    ptr = &sm83->reg_file.b;
    break;
  case 1:
    ptr = &sm83->reg_file.c;
    break;
  case 2:
    ptr = &sm83->reg_file.d;
    break;
  case 3:
    ptr = &sm83->reg_file.e;
    break;
  case 4:
    ptr = &sm83->reg_file.h;
    break;
  case 5:
    ptr = &sm83->reg_file.l;
    break;
  // case 6 is a memory address and must go through the memory interface
  case 7:
    ptr = &sm83->reg_file.a;
    break;
  default:
    ptr = &sm83->reg_file.b;
    break;
  }
  return ptr;
}

void load_reg_imm(sm83_t* sm83, uint8_t instr) {
  const uint8_t imm = sm83->io.mem_load(sm83->reg_file.pc);
  sm83->reg_file.pc++;
  const uint8_t target_idx = (SM83_MID_INDEX_MASK & instr) >> 3;
  // target_idx 6 is a write to memory
  if (target_idx == 6) {
    uint16_t addr = combine(sm83->reg_file.h, sm83->reg_file.l);
    sm83->io.mem_store(addr, imm);
    sm83->cycles++; // Extra cycle for mem access
  } else {
    uint8_t* target = reg_lookup_ptr(sm83, target_idx);
    *target = imm;
  }
  sm83->cycles += 2;
}

void push_rr(sm83_t* sm83, uint8_t instr) {
  uint16_t rr = 0;
  const uint8_t rr_mask = 0b00110000U;
  const uint8_t rr_index = (rr_mask & instr) >> 4;
  switch(rr_index) {
  case 0:
    rr = combine(sm83->reg_file.b, sm83->reg_file.c);
    break;
  case 1:
    rr = combine(sm83->reg_file.d, sm83->reg_file.e);
    break;
  case 2:
    rr = combine(sm83->reg_file.h, sm83->reg_file.l);
    break; 
  case 3:
    rr = combine(sm83->reg_file.a, sm83->reg_file.f);
    break;
  default:
    break;
  }
  uint8_t high = 0; 
  uint8_t low = 0; 
  separate(rr, &high, &low);
  sm83->reg_file.sp--;
  sm83->io.mem_store(sm83->reg_file.sp, high);
  sm83->reg_file.sp--;
  sm83->io.mem_store(sm83->reg_file.sp, low);
  sm83->cycles += 4;
}

void pop_rr(sm83_t* sm83, uint8_t instr) {
  const uint8_t low = sm83->io.mem_load(sm83->reg_file.sp);
  sm83->reg_file.sp++;
  const uint8_t high = sm83->io.mem_load(sm83->reg_file.sp);
  sm83->reg_file.sp++;
  const uint8_t rr_mask = 0b00110000U;
  const uint8_t rr_index = (rr_mask & instr) >> 4;
  switch(rr_index) {
  case 0:
    sm83->reg_file.b = high;
    sm83->reg_file.c = low;
    break;
  case 1:
    sm83->reg_file.d = high;
    sm83->reg_file.e = low;
    break;
  case 2:
    sm83->reg_file.h = high;
    sm83->reg_file.l = low;
    break;
  case 3:
    sm83->reg_file.a = high;
    sm83->reg_file.f = low;
    break;
  default:
    break;
  }
  sm83->cycles += 3;
}

uint16_t combine(uint8_t high, uint8_t low) {
  return ((uint16_t)high << 8) + (uint16_t)low;
}

// I'm not NULL check these pointers, they better be valid
void separate(uint16_t comb, uint8_t* high, uint8_t* low) {
  *low = (uint8_t)comb; // compiler might complain idk
  *high = (uint8_t)(comb >> 8);
}

void exec_cb_op(sm83_t* sm83, uint8_t cb_op) {
  const uint8_t cb_masked = cb_op & SM83_CB_MASK;
  const uint8_t cb_index = cb_op & SM83_END_INDEX_MASK;

  bool val_update = true;
  uint8_t val = 0;
  // All roads lead to switch
  switch(cb_masked){
    case SM83_RLC_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & (0x1U << 7);
      val = val << 1;
      if(carry){
        val |= 0x1U;
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    case SM83_RRC_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & 0x1U;
      val = val >> 1;
      if(carry){
        val |= 0x80U;
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    case SM83_RL_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & (0x1U << 7);
      val = val << 1;
      if(sm83->reg_file.f & FLAG_CARRY){
        val |= 0x1U;
      }
      if(carry){
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    case SM83_RR_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & 0x1U;
      val = val >> 1;
      if(sm83->reg_file.f & FLAG_CARRY){
        val |= 0x80U;
      }
      if(carry){
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    case SM83_SLA_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & (0x1U << 7);
      val = val << 1;
      if(carry){
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    case SM83_SRA_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & 0x1U;
      uint8_t b7 = val & (0x1U << 7);
      val = (val >> 1) | b7;
      if(carry){
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    case SM83_SWAP_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      val = (val << 4) | (val >> 4);
      sm83->reg_file.f &= ~FLAG_CARRY;
      break;
    }
    case SM83_SRL_PREFIX: {
      val = reg_lookup_val(sm83, cb_index);
      uint8_t carry = val & 0x1U;
      val = val >> 1;
      if(carry){
        sm83->reg_file.f |= FLAG_CARRY;
      }else{
        sm83->reg_file.f &= ~FLAG_CARRY;
      }
      break;
    }
    default:
      val_update = false;
      break;
  }
  if(val_update){
    if(cb_index == 6){ // HL indirect
      sm83->io.mem_store(combine(sm83->reg_file.h, sm83->reg_file.l), val);
      sm83->cycles++; // Extra cycle for mem store
    }else{
      uint8_t* reg = reg_lookup_ptr(sm83, cb_index);
      *reg = val;
    }
    if(val == 0){
      sm83->reg_file.f |= FLAG_ZERO;
    }else{
      sm83->reg_file.f &= ~FLAG_ZERO;
    }
    sm83->reg_file.f &= ~FLAG_SUB;
    sm83->reg_file.f &= ~FLAG_HALF_CARRY;
    sm83->cycles += 2;
  }

  uint8_t reg_idx = cb_op & SM83_END_INDEX_MASK;
  uint8_t bit = (cb_op & SM83_MID_INDEX_MASK) >> 3;
  uint8_t other_masked = cb_op & SM83_BIT_MASK;
  val_update = false;
  switch(other_masked){
    case SM83_BIT_PREFIX: {
      val = reg_lookup_val(sm83, reg_idx);
      if(val & (0x1U << bit)){
        sm83->reg_file.f &= ~FLAG_ZERO;
      }else{
        sm83->reg_file.f |= FLAG_ZERO;
      }
      sm83->reg_file.f &= ~FLAG_SUB;
      sm83->reg_file.f |= FLAG_HALF_CARRY;
      sm83->cycles += 2;
      break;
    }
    case SM83_RES_PREFIX: {
      val = reg_lookup_val(sm83, reg_idx);
      val &= ~(0x1U << bit);
      val_update = true;
      break;
    }
    case SM83_SET_PREFIX: {
      val = reg_lookup_val(sm83, reg_idx);
      val |= (0x1U << bit);
      val_update = true;
      break;
    }
    default: {
      // You shouldn't be here
      break;
    }
  }
  if(val_update){
    if(reg_idx == 6){ // HL indirect
      sm83->io.mem_store(combine(sm83->reg_file.h, sm83->reg_file.l), val);
      sm83->cycles++; // Extra cycle for mem store
    }else{
      uint8_t* reg = reg_lookup_ptr(sm83, cb_index);
      *reg = val;
    }
    sm83->cycles += 2;
  }
}

void jump_relative(sm83_t* sm83, int8_t e) {
  // Stupid implmentation idk
  if(e < 0){
    e *= -1;
    sm83->reg_file.pc -= (uint8_t)e;
  }else{
    sm83->reg_file.pc += (uint8_t)e;
  }
}

void call(sm83_t* sm83, uint16_t addr) {
  uint8_t pc_low = 0;
  uint8_t pc_high = 0;
  separate(sm83->reg_file.pc, &pc_high, &pc_low);
  sm83->reg_file.sp--;
  sm83->io.mem_store(sm83->reg_file.sp, pc_high);
  sm83->reg_file.sp--;
  sm83->io.mem_store(sm83->reg_file.sp, pc_low);
  sm83->reg_file.pc = addr;
}

void ret(sm83_t* sm83) {
  const uint8_t low = sm83->io.mem_load(sm83->reg_file.sp);
  sm83->reg_file.sp++;
  const uint8_t high = sm83->io.mem_load(sm83->reg_file.sp);
  sm83->reg_file.sp++;
  sm83->reg_file.pc = combine(high, low);
}

uint16_t load_nn(sm83_t* sm83) {
  const uint8_t nn_low = sm83->io.mem_load(sm83->reg_file.pc);
  sm83->reg_file.pc++;
  const uint8_t nn_high = sm83->io.mem_load(sm83->reg_file.pc);
  sm83->reg_file.pc++;
  return combine(nn_high, nn_low);
}

sm83_int_res_t handle_interrupt(sm83_t* sm83) {
  sm83_int_res_t retval = SM83_INT_RES_NOP;

  const uint8_t ie_val = sm83->io.mem_load(ADDR_IE);
  uint8_t if_val = sm83->io.mem_load(ADDR_IF);
  const uint8_t combined = ie_val & if_val;
  uint16_t handler_addr = 0;
  if(combined & INT_VBLANK){
    handler_addr = 0x0040U;
    if_val &= ~INT_VBLANK;
  }else if(combined & INT_LCD){
    handler_addr = 0x0048U;
    if_val &= ~INT_LCD;
  }else if(combined & INT_TIMER){
    handler_addr = 0x0050U;
    if_val &= ~INT_TIMER;
  }else if(combined & INT_SERIAL){
    handler_addr = 0x0058U;
    if_val &= ~INT_SERIAL;
  }else if(combined & INT_JOYPAD){
    handler_addr = 0x0060U;
    if_val &= ~INT_JOYPAD;
  }
  if(handler_addr){
    if(sm83->ime){
      sm83->ime = false;
      sm83->io.mem_store(ADDR_IF, if_val);
      call(sm83, handler_addr);
      sm83->cycles += 5;
      retval = SM83_INT_RES_EXEC;
    }else{
      retval = SM83_INT_RES_WAKE;
    }
  }
  return retval;
}