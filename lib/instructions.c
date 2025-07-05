#include <instructions.h>
#include <cpu.h>

instruction instructions[0x100] = {
    [0x00] = {IN_NOP, AM_IMP},
    [0x01] = {IN_LD, AM_R_D16, RT_BC},
    [0x02] = {IN_LD, AM_MR_R, RT_BC, RT_A},
    
    [0x05] = {IN_DEC, AM_R, RT_B},
    [0x06] = {IN_LD, AM_R_D8, RT_B},

    [0x08] = {IN_LD, AM_A16_R, RT_SP},

    [0x0A] = {IN_LD},
    
    [0x0E] = {IN_LD, AM_R_D8, RT_C},
    
    [0xAF] = {IN_XOR, AM_R, RT_A},
    
    [0xC3] = {IN_JP, AM_D16},
    
    [0xF3] = {IN_DI},
};

instruction *instruction_by_opcode(u8 opcode) {
    return &instructions[opcode];
};

char *inst_lookup[] = {
    "<NONE>",
    "NOP",
    "LD",
    "INC",
    "DEC",
    "RLCA",
    "ADD",
    "RRCA",
    "STOP",
    "RLA",
    "JR",
    "RRA",
    "DAA",
    "CPL",
    "SCF",
    "CCF",
    "HALT",
    "ADC",
    "SUB",
    "SBC",
    "AND",
    "XOR",
    "OR",
    "CP",
    "POP",
    "JP",
    "PUSH",
    "RET",
    "CB",
    "CALL",
    "RETI",
    "LDH",
    "JPHL",
    "DI",
    "EI",
    "RST",
    "IN_ERR",
    "IN_RLC", 
    "IN_RRC",
    "IN_RL", 
    "IN_RR",
    "IN_SLA", 
    "IN_SRA",
    "IN_SWAP", 
    "IN_SRL",
    "IN_BIT", 
    "IN_RES", 
    "IN_SET"
};

char *inst_name(in_type t) {
    return inst_lookup[t];
}