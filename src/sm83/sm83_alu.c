#include "sm83_alu.h"
#include "sm83_common.h"

static uint8_t set_flags(sm83_alu_op_t op, uint8_t flags, uint8_t result, uint8_t carry);

void sm83_alu_execute(sm83_alu_op_t op, sm83_alu_t* operands) {
  uint8_t carry = 0;
  switch(op) {
    case SM83_ALU_OP_ADD: {
      carry = operands->primary & operands->secondary;
      operands->primary += operands->secondary;
      break;
    }
    case SM83_ALU_OP_ADC: {
      const uint8_t c_val = ((operands->flags & FLAG_CARRY) ? 1U : 0U);
      carry = operands->primary & operands->secondary & c_val;
      operands->primary += (operands->secondary + c_val);
      break;
    }
    case SM83_ALU_OP_SUB: {
      const uint8_t twos_comp = (~operands->secondary) + 1U;
      carry = (operands->primary & twos_comp);
      operands->primary += twos_comp;
      break;
    }
    case SM83_ALU_OP_SBC: {
      const uint8_t twos_comp = (~operands->secondary) + 1U;
      const uint8_t c_val = ((operands->flags & FLAG_CARRY) ? 1U : 0U);
      carry = (operands->primary & twos_comp & c_val);
      // This might be wrong
      operands->primary += twos_comp;
      operands->primary += c_val;
      break;
    }
    case SM83_ALU_OP_INC: {
      carry = (operands->primary & 1U);
      operands->primary++;
      break;
    }
    case SM83_ALU_OP_DEC: {
      carry = (operands->primary & 255U);
      operands->primary += 255U;
      break;
    }
    case SM83_ALU_OP_AND: {
      operands->primary &= operands->secondary;
      break;
    }
    case SM83_ALU_OP_OR: {
      operands->primary |= operands->secondary;
      break;
    }
    case SM83_ALU_OP_XOR: {
      operands->primary ^= operands->secondary;
      break;
    }
    default:
      break;
  }

  operands->flags = set_flags(op, operands->flags, operands->primary, carry);
}

uint8_t set_flags(sm83_alu_op_t op, uint8_t flags, uint8_t result, uint8_t carry) {
  uint8_t retval = flags;
  if(result == 0) {
    retval |= FLAG_ZERO;
  } else {
    retval &= ~FLAG_ZERO;
  }
  switch(op) {
    case SM83_ALU_OP_ADD:
    case SM83_ALU_OP_ADC:
      retval &= ~FLAG_SUB;
      if (carry & MASK_HALF_CARRY) {
        retval |= FLAG_HALF_CARRY;
      } else {
        retval &= ~FLAG_HALF_CARRY;
      }
      if (carry & MASK_CARRY) {
        retval |= FLAG_CARRY;
      } else {
        retval &= ~FLAG_CARRY;
      }
      break;
    case SM83_ALU_OP_SUB:
    case SM83_ALU_OP_SBC:
      retval |= FLAG_SUB;
      if (carry & MASK_HALF_CARRY) {
        retval |= FLAG_HALF_CARRY;
      } else {
        retval &= ~FLAG_HALF_CARRY;
      }
      if (carry & MASK_CARRY) {
        retval |= FLAG_CARRY;
      } else {
        retval &= ~FLAG_CARRY;
      }
      break;
    case SM83_ALU_OP_INC:
      retval &= ~FLAG_SUB;
      if (carry & MASK_CARRY) {
        retval |= FLAG_CARRY;
      } else {
        retval &= ~FLAG_CARRY;
      }
      break;
    case SM83_ALU_OP_DEC:
      retval |= FLAG_SUB;
      if (carry & MASK_CARRY) {
        retval |= FLAG_CARRY;
      } else {
        retval &= ~FLAG_CARRY;
      }
      break;
    case SM83_ALU_OP_AND:
      retval &= ~FLAG_SUB;
      retval |= FLAG_HALF_CARRY;
      retval &= ~FLAG_CARRY;
      break;
    case SM83_ALU_OP_XOR:
      retval &= ~FLAG_SUB;
      retval &= ~FLAG_HALF_CARRY;
      retval &= ~FLAG_CARRY;
      break;
    default:
      break;
  }
  return retval;
}

void sm83_alu16_execute(sm83_alu_op_t op, sm83_alu16_t* operands) {
  switch(op) {
    case SM83_ALU_OP_ADD: {
      uint16_t carry = operands->primary & operands->secondary;
      operands->primary += operands->secondary;
      operands->flags &= ~FLAG_SUB;
      if (carry & (1U << 11)) {
        operands->flags |= FLAG_HALF_CARRY;
      } else {
        operands->flags &= ~FLAG_HALF_CARRY;
      }
      if (carry & (1U << 15)) {
        operands->flags |= FLAG_CARRY;
      } else {
        operands->flags &= ~FLAG_CARRY;
      }
      break;
    }
    default:
      break;
  }
}