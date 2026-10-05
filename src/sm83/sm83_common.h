#ifndef SM83_COMMON_H
#define SM83_COMMON_H

// arithmetic flags (f register)
#define FLAG_ZERO (1U << 7)
#define FLAG_SUB  (1U << 6)
#define FLAG_HALF_CARRY (1U << 5)
#define FLAG_CARRY (1U << 4)

#define MASK_HALF_CARRY (1U << 7)
#define MASK_CARRY (1U << 3)

enum {
  // 0x00
  SM83_INSTR_NOP = 0U,      // NOP
  SM83_INSTR_LD_BC_nn,      // LD BC,nn
  SM83_INSTR_LD_pBC_A,      // LD (BC),A
  SM83_INSTR_INC_BC,        // INC BC
  SM83_INSTR_INC_B,         // INC B
  SM83_INSTR_DEC_B,         // DEC B
  SM83_INSTR_LD_B_n,        // LD B,n
  SM83_INSTR_RLCA,          // RLCA
  SM83_INSTR_LD_pnn_SP,     // LD (nn),SP
  SM83_INSTR_ADD_HL_BC,     // ADD HL,BC
  SM83_INSTR_LD_A_pBC,      // LD A,(BC)
  SM83_INSTR_DEC_BC,        // DEC BC
  SM83_INSTR_INC_C,         // INC C
  SM83_INSTR_DEC_C,         // DEC C
  SM83_INSTR_LD_C_n,        // LD C,n
  SM83_INSTR_RRCA,          // RRCA
  // 0x10
  SM83_INSTR_STOP,          // STOP
  SM83_INSTR_LD_DE_nn,      // LD DE,nn
  SM83_INSTR_LD_pDE_A,      // LD (DE),A
  SM83_INSTR_INC_DE,        // INC DE
  SM83_INSTR_INC_D,         // INC D
  SM83_INSTR_DEC_D,         // DEC D
  SM83_INSTR_LD_D_n,        // LD D,n
  SM83_INSTR_RLA,           // RLA
  SM83_INSTR_JR_e,          // JR e
  SM83_INSTR_ADD_HL_DE,     // ADD HL,DE
  SM83_INSTR_LD_A_pDE,      // LD A,(DE)
  SM83_INSTR_DEC_DE,        // DEC DE
  SM83_INSTR_INC_E,         // INC E
  SM83_INSTR_DEC_E,         // DEC E
  SM83_INSTR_LD_E_n,        // LD E,n
  SM83_INSTR_RRA,           // RRA
  // 0x20
  SM83_INSTR_JR_NZ_e,       // JR NZ,e
  SM83_INSTR_LD_HL_nn,      // LD HL,nn
  SM83_INSTR_LD_pHLp_A,     // LD (HL+),A
  SM83_INSTR_INC_HL,        // INC HL
  SM83_INSTR_INC_H,         // INC H
  SM83_INSTR_DEC_H,         // DEC H
  SM83_INSTR_LD_H_n,        // LD H,n
  SM83_INSTR_DAA,           // DAA
  SM83_INSTR_JR_Z_e,        // JR Z,e
  SM83_INSTR_ADD_HL_HL,     // ADD HL,HL
  SM83_INSTR_LD_A_pHLp,     // LD A,(HL+)
  SM83_INSTR_DEC_HL,        // DEC HL
  SM83_INSTR_INC_L,         // INC L
  SM83_INSTR_DEC_L,         // DEC L
  SM83_INSTR_LD_L_n,        // LD L,n
  SM83_INSTR_CPL,           // CPL
  // 0x30
  SM83_INSTR_JR_NC_e,       // JR NC,e
  SM83_INSTR_LD_SP_nn,      // LD SP,nn
  SM83_INSTR_LD_pHLm_A,     // LD (HL-),A
  SM83_INSTR_INC_SP,        // INC SP
  SM83_INSTR_INC_pHL,       // INC (HL)
  SM83_INSTR_DEC_pHL,       // DEC (HL)
  SM83_INSTR_LD_pHL_n,      // LD (HL),n
  SM83_INSTR_SCF,           // SCF
  SM83_INSTR_JR_C_e,        // JR C,e
  SM83_INSTR_ADD_HL_SP,     // ADD HL,SP
  SM83_INSTR_LD_A_pHLm,     // LD A,(HL-)
  SM83_INSTR_DEC_SP,        // DEC SP
  SM83_INSTR_INC_A,         // INC A
  SM83_INSTR_DEC_A,         // DEC A
  SM83_INSTR_LD_A_n,        // LD A,n
  SM83_INSTR_CCF,           // CCF
  // 0x40
  SM83_INSTR_LD_B_B,        // LD B,B
  SM83_INSTR_LD_B_C,        // LD B,C
  SM83_INSTR_LD_B_D,        // LD B,D
  SM83_INSTR_LD_B_E,        // LD B,E
  SM83_INSTR_LD_B_H,        // LD B,H
  SM83_INSTR_LD_B_L,        // LD B,L
  SM83_INSTR_LD_B_pHL,      // LD B,(HL)
  SM83_INSTR_LD_B_A,        // LD B,A
  SM83_INSTR_LD_C_B,        // LD C,B
  SM83_INSTR_LD_C_C,        // LD C,C
  SM83_INSTR_LD_C_D,        // LD C,D
  SM83_INSTR_LD_C_E,        // LD C,E
  SM83_INSTR_LD_C_H,        // LD C,H
  SM83_INSTR_LD_C_L,        // LD C,L
  SM83_INSTR_LD_C_pHL,      // LD C,(HL)
  SM83_INSTR_LD_C_A,        // LD C,A
  // 0x50
  SM83_INSTR_LD_D_B,        // LD D,B
  SM83_INSTR_LD_D_C,        // LD D,C
  SM83_INSTR_LD_D_D,        // LD D,D
  SM83_INSTR_LD_D_E,        // LD D,E
  SM83_INSTR_LD_D_H,        // LD D,H
  SM83_INSTR_LD_D_L,        // LD D,L
  SM83_INSTR_LD_D_pHL,      // LD D,(HL)
  SM83_INSTR_LD_D_A,        // LD D,A
  SM83_INSTR_LD_E_B,        // LD E,B
  SM83_INSTR_LD_E_C,        // LD E,C
  SM83_INSTR_LD_E_D,        // LD E,D
  SM83_INSTR_LD_E_E,        // LD E,E
  SM83_INSTR_LD_E_H,        // LD E,H
  SM83_INSTR_LD_E_L,        // LD E,L
  SM83_INSTR_LD_E_pHL,      // LD E,(HL)
  SM83_INSTR_LD_E_A,        // LD E,A
  // 0x60
  SM83_INSTR_LD_H_B,        // LD H,B
  SM83_INSTR_LD_H_C,        // LD H,C
  SM83_INSTR_LD_H_D,        // LD H,D
  SM83_INSTR_LD_H_E,        // LD H,E
  SM83_INSTR_LD_H_H,        // LD H,H
  SM83_INSTR_LD_H_L,        // LD H,L
  SM83_INSTR_LD_H_pHL,      // LD H,(HL)
  SM83_INSTR_LD_H_A,        // LD H,A
  SM83_INSTR_LD_L_B,        // LD L,B
  SM83_INSTR_LD_L_C,        // LD L,C
  SM83_INSTR_LD_L_D,        // LD L,D
  SM83_INSTR_LD_L_E,        // LD L,E
  SM83_INSTR_LD_L_H,        // LD L,H
  SM83_INSTR_LD_L_L,        // LD L,L
  SM83_INSTR_LD_L_pHL,      // LD L,(HL)
  SM83_INSTR_LD_L_A,        // LD L,A
  // 0x70
  SM83_INSTR_LD_pHL_B,      // LD (HL),B
  SM83_INSTR_LD_pHL_C,      // LD (HL),C
  SM83_INSTR_LD_pHL_D,      // LD (HL),D
  SM83_INSTR_LD_pHL_E,      // LD (HL),E
  SM83_INSTR_LD_pHL_H,      // LD (HL),H
  SM83_INSTR_LD_pHL_L,      // LD (HL),L
  SM83_INSTR_HALT,          // HALT
  SM83_INSTR_LD_pHL_A,      // LD (HL),A
  SM83_INSTR_LD_A_B,        // LD A,B
  SM83_INSTR_LD_A_C,        // LD A,C
  SM83_INSTR_LD_A_D,        // LD A,D
  SM83_INSTR_LD_A_E,        // LD A,E
  SM83_INSTR_LD_A_H,        // LD A,H
  SM83_INSTR_LD_A_L,        // LD A,L
  SM83_INSTR_LD_A_pHL,      // LD A,(HL)
  SM83_INSTR_LD_A_A,        // LD A,A
  // 0x80
  SM83_INSTR_ADD_B,         // ADD B
  SM83_INSTR_ADD_C,         // ADD C
  SM83_INSTR_ADD_D,         // ADD D
  SM83_INSTR_ADD_E,         // ADD E
  SM83_INSTR_ADD_H,         // ADD H
  SM83_INSTR_ADD_L,         // ADD L
  SM83_INSTR_ADD_pHL,       // ADD (HL)
  SM83_INSTR_ADD_A,         // ADD A
  SM83_INSTR_ADC_B,         // ADC B
  SM83_INSTR_ADC_C,         // ADC C
  SM83_INSTR_ADC_D,         // ADC D
  SM83_INSTR_ADC_E,         // ADC E
  SM83_INSTR_ADC_H,         // ADC H
  SM83_INSTR_ADC_L,         // ADC L
  SM83_INSTR_ADC_pHL,       // ADC (HL)
  SM83_INSTR_ADC_A,         // ADC A
  // 0x90
  SM83_INSTR_SUB_B,         // SUB B
  SM83_INSTR_SUB_C,         // SUB C
  SM83_INSTR_SUB_D,         // SUB D
  SM83_INSTR_SUB_E,         // SUB E
  SM83_INSTR_SUB_H,         // SUB H
  SM83_INSTR_SUB_L,         // SUB L
  SM83_INSTR_SUB_pHL,       // SUB (HL)
  SM83_INSTR_SUB_A,         // SUB A
  SM83_INSTR_SBC_B,         // SBC B
  SM83_INSTR_SBC_C,         // SBC C
  SM83_INSTR_SBC_D,         // SBC D
  SM83_INSTR_SBC_E,         // SBC E
  SM83_INSTR_SBC_H,         // SBC H
  SM83_INSTR_SBC_L,         // SBC L
  SM83_INSTR_SBC_pHL,       // SBC (HL)
  SM83_INSTR_SBC_A,         // SBC A
  // 0xa0
  SM83_INSTR_AND_B,         // AND B
  SM83_INSTR_AND_C,         // AND C
  SM83_INSTR_AND_D,         // AND D
  SM83_INSTR_AND_E,         // AND E
  SM83_INSTR_AND_H,         // AND H
  SM83_INSTR_AND_L,         // AND L
  SM83_INSTR_AND_pHL,       // AND (HL)
  SM83_INSTR_AND_A,         // AND A
  SM83_INSTR_XOR_B,         // XOR B
  SM83_INSTR_XOR_C,         // XOR C
  SM83_INSTR_XOR_D,         // XOR D
  SM83_INSTR_XOR_E,         // XOR E
  SM83_INSTR_XOR_H,         // XOR H
  SM83_INSTR_XOR_L,         // XOR L
  SM83_INSTR_XOR_pHL,       // XOR (HL)
  SM83_INSTR_XOR_A,         // XOR A
  // 0xb0
  SM83_INSTR_OR_B,          // OR B
  SM83_INSTR_OR_C,          // OR C
  SM83_INSTR_OR_D,          // OR D
  SM83_INSTR_OR_E,          // OR E
  SM83_INSTR_OR_H,          // OR H
  SM83_INSTR_OR_L,          // OR L
  SM83_INSTR_OR_pHL,        // OR (HL)
  SM83_INSTR_OR_A,          // OR A
  SM83_INSTR_CP_B,          // CP B
  SM83_INSTR_CP_C,          // CP C
  SM83_INSTR_CP_D,          // CP D
  SM83_INSTR_CP_E,          // CP E
  SM83_INSTR_CP_H,          // CP H
  SM83_INSTR_CP_L,          // CP L
  SM83_INSTR_CP_pHL,        // CP (HL)
  SM83_INSTR_CP_A,          // CP A
  // 0xc0
  SM83_INSTR_RET_NZ,        // RET NZ
  SM83_INSTR_POP_BC,        // POP BC
  SM83_INSTR_JP_NZ_nn,      // JP NZ,nn
  SM83_INSTR_JP_nn,         // JP nn
  SM83_INSTR_CALL_NZ_nn,    // CALL NZ,nn
  SM83_INSTR_PUSH_BC,       // PUSH BC
  SM83_INSTR_ADD_n,         // ADD n
  SM83_INSTR_RST_0x00,      // RST 0x00
  SM83_INSTR_RET_Z,         // RET Z
  SM83_INSTR_RET,           // RET
  SM83_INSTR_JP_Z_nn,       // JP Z,nn
  SM83_INSTR_CB_op,         // CB op
  SM83_INSTR_CALL_Z_nn,     // CALL Z,nn
  SM83_INSTR_CALL_nn,       // CALL nn
  SM83_INSTR_ADC_n,         // ADC n
  SM83_INSTR_RST_0x08,      // RST 0x08
  // 0xd0
  SM83_INSTR_RET_NC,        // RET NC
  SM83_INSTR_POP_DE,        // POP DE
  SM83_INSTR_JP_NC_nn,      // JP NC,nn
  SM83_INSTR_UND_0xd3,      // undefined 0xd3
  SM83_INSTR_CALL_NC_nn,    // CALL NC,nn
  SM83_INSTR_PUSH_DE,       // PUSH DE
  SM83_INSTR_SUB_n,         // SUB n
  SM83_INSTR_RST_0x10,      // RST 0x10
  SM83_INSTR_RET_C,         // RET C
  SM83_INSTR_RETI,          // RETI
  SM83_INSTR_JP_C_nn,       // JP C,nn
  SM83_INSTR_UND_0xdb,      // undefined 0xdb
  SM83_INSTR_CALL_C_nn,     // CALL C,nn
  SM83_INSTR_UND_0xdd,      // undefined 0xdd
  SM83_INSTR_SBC_n,         // SBC n
  SM83_INSTR_RST_0x18,      // RST 0x18
  // 0xe0
  SM83_INSTR_LDH_pn_A,      // LDH (n),A
  SM83_INSTR_POP_HL,        // POP HL
  SM83_INSTR_LDH_pC_A,      // LDH (C),A
  SM83_INSTR_UND_0xe3,      // undefined 0xe3
  SM83_INSTR_UND_0xe4,      // undefined 0xe4
  SM83_INSTR_PUSH_HL,       // PUSH HL
  SM83_INSTR_AND_n,         // AND n
  SM83_INSTR_RST_0x20,      // RST 0x20
  SM83_INSTR_ADD_SP_n,      // ADD SP,n
  SM83_INSTR_JP_HL,         // JP HL
  SM83_INSTR_LD_pnn_A,      // LD (nn),A
  SM83_INSTR_UND_0xeb,      // undefined 0xeb
  SM83_INSTR_UND_0xec,      // undefined 0xec
  SM83_INSTR_UND_0xed,      // undefined 0xed
  SM83_INSTR_XOR_n,         // XOR n
  SM83_INSTR_RST_0x28,      // RST 0x28
  // 0xf0
  SM83_INSTR_LDH_A_pn,      // LDH A,(n)
  SM83_INSTR_POP_AF,        // POP AF
  SM83_INSTR_LDH_A_pC,      // LDH A,(C)
  SM83_INSTR_DI,            // DI
  SM83_INSTR_UND_0xf4,      // undefined 0xf4
  SM83_INSTR_PUSH_AF,       // PUSH AF
  SM83_INSTR_OR_n,          // OR n
  SM83_INSTR_RST_0x30,      // RST 0x30
  SM83_INSTR_LD_HL_SPpe,    // LD HL_SP+e
  SM83_INSTR_LD_SP_HL,      // LD SP,HL
  SM83_INSTR_LD_A_pnn,      // LD A,(nn)
  SM83_INSTR_EI,            // EI
  SM83_INSTR_UND_0xfc,      // undefined 0xfc
  SM83_INSTR_UND_0xfd,      // undefined 0xfd
  SM83_INSTR_CP_n,          // CP n
  SM83_INSTR_RST_0x38,      // RST 0x38
  NUM_SM83_INSTR
};

#endif