#ifndef SM83_ALU_H_
#define SM83_ALU_H_

#include <stdint.h>

typedef enum {
  SM83_ALU_OP_ADD,
  SM83_ALU_OP_ADC,
  SM83_ALU_OP_SUB,
  SM83_ALU_OP_SBC,
  SM83_ALU_OP_INC,
  SM83_ALU_OP_DEC,
  SM83_ALU_OP_AND,
  SM83_ALU_OP_OR,
  SM83_ALU_OP_XOR,
  NUM_SM83_ALU_OP
} sm83_alu_op_t;

typedef struct sm83_alu_s {
  uint8_t primary;
  uint8_t secondary;
  uint8_t flags;
}sm83_alu_t;

typedef struct sm83_alu16_s {
  uint16_t primary;
  uint16_t secondary;
  uint8_t flags;
}sm83_alu16_t;

/*
* Performs an ALU operation on the operands,
* modifying operands.primary and operands.flags in the process
*/
void sm83_alu_execute(sm83_alu_op_t op, sm83_alu_t* operands);

void sm83_alu16_execute(sm83_alu_op_t op, sm83_alu16_t* operands);

#endif