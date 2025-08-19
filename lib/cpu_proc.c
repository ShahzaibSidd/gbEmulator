#include <cpu.h>
#include <emu.h>
#include <bus.h>
#include <stack.h>

//where the emulator processes cpu intructions

void cpu_set_flags(cpu_context *ctx, char z, char n, char h, char c) {
    if (z != -1) {
        BIT_SET(ctx->regs.f, 7, z);
    }
    if (n != -1) {
        BIT_SET(ctx->regs.f, 6, n);
    }
    if (h != -1) {
        BIT_SET(ctx->regs.f, 5, h);
    }
    if (c != -1) {
        BIT_SET(ctx->regs.f, 4, c);
    }
}

static bool check_cond(cpu_context *ctx) {
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(ctx->cur_inst->cond) {
        case CT_NONE: return true;
        case CT_C: return c;
        case CT_NC: return !c;
        case CT_Z: return z;
        case CT_NZ: return !z;
    }   
    return false;
}

static bool is_16bit(reg_type rt) {
    return rt >= RT_AF;
}

reg_type rt_lookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A
};

reg_type decode_reg(u8 val) {
    if (val > 0b0111) {
        return RT_NONE;
    }
    return rt_lookup[val];
}

static void proc_none(cpu_context *ctx) {
    printf("INVALID INSTRUCTION OR NOT IMPLEMENTED YET\n");
    exit(-7);
}

static void proc_nop(cpu_context *ctx) {
    u8 op = ctx->fetched_data;
    reg_type reg = decode_reg((op & 0b111));
    return;
}

static void proc_cb(cpu_context *ctx) {
    u8 data = ctx->fetched_data;
    reg_type rt = decode_reg(data & 0b111);

    //bits 3-5
    u8 count_bits = (data >> 3) & 0b111;
    //bits 6-7
    u8 top_two_bits = (data >> 6) & 0b11;

    u8 reg_value = cpu_read_reg8(rt);

    emu_cycles(1);

    if (rt == RT_HL) {
        emu_cycles(2);
    }

    switch (top_two_bits) {
        case 1:
            //BIT instruction
            cpu_set_flags(ctx, !(reg_value & (1 << count_bits)), 0, 1, -1);
            return;
        
        case 2:
            //RES intruction
            reg_value &= ~(1 << count_bits);
            cpu_set_reg8(rt, reg_value);
            return;
        
        case 3:
            //SET instruction
            reg_value |= (1 << count_bits);
            cpu_set_reg8(rt, reg_value);
            return;
    }

    bool flag_c = CPU_FLAG_C;
    
    switch (count_bits) {
        case 0: {
            //RLC instruction
            bool bit7 = (reg_value & (1 << 7)) != 0;
            u8 new_val = (reg_value << 1) | bit7;
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit7);
        } return;

        case 1: {
            //RRC instruction
            u8 bit0 = (reg_value & 1);
            u8 new_val = (reg_value >> 1) | (bit0 << 7);
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit0);
        } return;

        case 2: {
            //RL instruction
            bool bit7 = (reg_value & (1 << 7)) != 0;
            u8 new_val = (reg_value << 1) | flag_c;
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit7);
        } return;

        case 3: {
            //RR instruction
            u8 bit0 = (reg_value & 1);
            u8 new_val = (reg_value >> 1) | (flag_c << 7);
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit0);
        } return;

        case 4: {
            //SLA instruction
            bool bit7 = (reg_value & (1 << 7)) != 0;
            u8 new_val = (reg_value << 1);
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit7);
        } return;

        case 5: {
            //SRA instruction
            u8 bit0 = (reg_value & 1);
            u8 bit7 = reg_value & (1 << 7);
            u8 new_val = (reg_value >> 1) | bit7;
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit0);
        } return;

        case 6: {
            //SWAP instruction
            u8 nibble0 = reg_value & 0x0F;
            u8 nibble1 = reg_value & 0xF0;
            u8 new_val = (nibble0 << 4) | (nibble1 >> 4);
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, 0);
        } return;

        case 7: {
            //SRL instruction
            u8 bit0 = (reg_value & 1);
            u8 new_val = (reg_value >> 1);
            cpu_set_reg8(rt, new_val);

            cpu_set_flags(ctx, new_val == 0, 0, 0, bit0);
        } return;
    }

    fprintf(stderr, "ERROR: CB INSTRUCTION DOESN'T EXIST: %02X", data);
    NOT_IMPL
}

static void proc_rlca(cpu_context *ctx) {
    u8 reg_value = cpu_read_reg8(RT_A);

    bool bit7 = (reg_value & (1 << 7)) != 0;
    u8 new_val = (reg_value << 1) | bit7;

    cpu_set_reg8(RT_A, new_val);
    cpu_set_flags(ctx, 0, 0, 0, bit7);
}

static void proc_rrca(cpu_context *ctx) {
    u8 reg_value = cpu_read_reg8(RT_A);

    u8 bit0 = (reg_value & 1);
    u8 new_val = (reg_value >> 1) | (bit0 << 7);
    cpu_set_reg8(RT_A, new_val);
    cpu_set_flags(ctx, 0, 0, 0, bit0);
}

static void proc_rla(cpu_context *ctx) {
    u8 reg_value = cpu_read_reg8(RT_A);
    u8 flag_c = CPU_FLAG_C;

    bool bit7 = (reg_value & (1 << 7)) != 0;
    u8 new_val = (reg_value << 1) | flag_c;
    cpu_set_reg8(RT_A, new_val);
    cpu_set_flags(ctx, 0, 0, 0, bit7);
}

static void proc_rra(cpu_context *ctx) {
    u8 reg_value = cpu_read_reg8(RT_A);
    u8 flag_c = CPU_FLAG_C;

    u8 bit0 = (reg_value & 1);
    u8 new_val = (reg_value >> 1) | (flag_c << 7);
    cpu_set_reg8(RT_A, new_val);
    cpu_set_flags(ctx, 0, 0, 0, bit0);
}

static void proc_and(cpu_context *ctx) {
    u8 val = cpu_read_reg(RT_A) & ctx->fetched_data;
    cpu_set_reg(RT_A, val);
    cpu_set_flags(ctx, val==0, 0, 1, 0);
}

static void proc_xor(cpu_context *ctx) {
    u8 val = cpu_read_reg(RT_A) ^ (ctx->fetched_data & 0xFF);
    cpu_set_reg(RT_A, val);
    cpu_set_flags(ctx, val==0, 0, 0, 0);
}

static void proc_or(cpu_context *ctx) {
    u8 val = cpu_read_reg(RT_A) | (ctx->fetched_data & 0xFF);
    cpu_set_reg(RT_A, val);
    cpu_set_flags(ctx, val==0, 0, 0, 0);
}

static void proc_cp(cpu_context *ctx) {
    int val = cpu_read_reg(RT_A) - ctx->fetched_data;

    int z = val == 0;
    int h = (cpu_read_reg(RT_A) & 0x0F) < (ctx->fetched_data & 0x0F);
    int c = val < 0;

    cpu_set_flags(ctx, z, 1, h, c);
}

static void proc_ld(cpu_context *ctx) {
    if (ctx->dest_is_mem) {
        if (is_16bit(ctx->cur_inst->reg_2)) {
            emu_cycles(1);
            bus_write16(ctx->mem_dest, ctx->fetched_data);
        } else {
            bus_write(ctx->mem_dest, ctx->fetched_data);
        }

        emu_cycles(1);
        return;
    }

    if (ctx->cur_inst->mode == AM_HL_SPR) {
        u8 hflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xF) + 
            (ctx->fetched_data & 0xF) >= 0x10;

        u8 cflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xFF) + 
            (ctx->fetched_data & 0xFF) >= 0x100;

        cpu_set_flags(ctx, 0, 0, hflag, cflag);
        cpu_set_reg(ctx->cur_inst->reg_1, 
            cpu_read_reg(ctx->cur_inst->reg_2) + (char)ctx->fetched_data);

        return;
    }

    cpu_set_reg(ctx->cur_inst->reg_1, ctx->fetched_data);
}

static void proc_ldh(cpu_context *ctx) {
    if (ctx->cur_inst->reg_1 == RT_A) {
        cpu_set_reg(ctx->cur_inst->reg_1, bus_read(0xFF00 | ctx->fetched_data));
    } else {
        bus_write(ctx->mem_dest, ctx->regs.a);
    }

    emu_cycles(1);
}

static void goto_addr(cpu_context *ctx, u16 addr, bool pushpc) {
    if (check_cond(ctx)) {
        if (pushpc) {
            emu_cycles(2);
            stack_push16(ctx->regs.pc);
        }

        ctx->regs.pc = addr;
        emu_cycles(1);
    }
}

static void proc_jp(cpu_context *ctx) {
    goto_addr(ctx, ctx->fetched_data, false);
}

static void proc_jr(cpu_context *ctx) {
    char temp = (char)(ctx->fetched_data) & 0xFF;
    u16 address = ctx->regs.pc + temp;
    goto_addr(ctx, address, false);
}

static void proc_call(cpu_context *ctx) {
    goto_addr(ctx, ctx->fetched_data, true);
}

static void proc_rst(cpu_context *ctx) {
    goto_addr(ctx, ctx->cur_inst->param, true);
}

static void proc_ret(cpu_context *ctx) {
    if (ctx->cur_inst->cond != CT_NONE) {
        emu_cycles(1);
    }

    if (check_cond(ctx)) {
        u16 address = stack_pop16();
        emu_cycles(2);

        ctx->regs.pc = address;

        emu_cycles(1);
    }
}

static void proc_reti(cpu_context *ctx) {
    //same as proc_ret, just enabling the interrupt variable;
    ctx->int_master_enabled = true;
    proc_ret(ctx);
}

static void proc_pop(cpu_context *ctx) {
    u16 val = stack_pop16();
    emu_cycles(1);

    cpu_set_reg(ctx->cur_inst->reg_1, val);
    if (ctx->cur_inst->reg_1 == RT_AF) {
        cpu_set_reg(ctx->cur_inst->reg_1, val & 0xFFF0);
    }
}

static void proc_push(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1);    
    emu_cycles(1);
    stack_push16(val);
    emu_cycles(2);

    return;
}

static void proc_inc(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) + 1;

    if (is_16bit(ctx->cur_inst->reg_1)) {
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR) {
        val = bus_read(cpu_read_reg(RT_HL)) + 1;
        val &= 0xFF; 
        bus_write(cpu_read_reg(RT_HL), val);
    } else {
        cpu_set_reg(ctx->cur_inst->reg_1, val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }

    if ((ctx->cur_opcode & 0x03) == 0x03) {
        return;
    }

    cpu_set_flags(ctx, val == 0, 0, (val & 0x0F) == 0, -1);
}

static void proc_dec(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - 1;

    if (is_16bit(ctx->cur_inst->reg_1)) {
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR) {
        val = bus_read(cpu_read_reg(RT_HL)) - 1;
        val &= 0xFF;
        bus_write(cpu_read_reg(RT_HL), val);
    } else {
        cpu_set_reg(ctx->cur_inst->reg_1, val);
        val = cpu_read_reg(ctx->cur_inst->reg_1);
    }

    if ((ctx->cur_opcode & 0x0B) == 0x0B) {
        return;
    }

    cpu_set_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
}

static void proc_sub(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - ctx->fetched_data;
    
    int z = val == 0;
    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) - ((int)(ctx->fetched_data) & 0xF) < 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) - ((int)(ctx->fetched_data)) < 0;
    
    cpu_set_reg(ctx->cur_inst->reg_1, val);
    cpu_set_flags(ctx,z, 1, h, c);
}

static void proc_sbc(cpu_context *ctx) {
    u8 val = ctx->fetched_data + CPU_FLAG_C;

    int z = cpu_read_reg(RT_A) - val == 0;
    int h = ((int)cpu_read_reg(RT_A) & 0xF) - ((int)(ctx->fetched_data) & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)cpu_read_reg(RT_A)) - ((int)(ctx->fetched_data)) - ((int)CPU_FLAG_C) < 0;

    cpu_set_reg(RT_A, cpu_read_reg(RT_A) - val);
    cpu_set_flags(ctx,z, 1, h, c);
}

static void proc_adc(cpu_context *ctx) {
    u16 u = ctx->fetched_data;
    u16 a = cpu_read_reg(RT_A);
    u16 carry_flag = CPU_FLAG_C;

    cpu_set_reg(RT_A, u+a+carry_flag);

    int z = (u+a+carry_flag & 0xFF) == 0;
    int h = (a & 0xF) + (u & 0xF) + carry_flag > 0xF;
    int c_new = a+u+carry_flag > 0xFF;

    cpu_set_flags(ctx, z, 0, h, c_new);
}

static void proc_add(cpu_context *ctx) {
    u32 val = cpu_read_reg(ctx->cur_inst->reg_1) + ctx->fetched_data;

    bool is_16_bit = is_16bit(ctx->cur_inst->reg_1);

    if (is_16_bit) {
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {
        val = cpu_read_reg(ctx->cur_inst->reg_1) + (char)ctx->fetched_data;
    }

    int z = (val & 0xFF) == 0;
    int h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
    int c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;

    if (is_16_bit) {
        z = -1;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xFFF) + (ctx->fetched_data & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpu_read_reg(ctx->cur_inst->reg_1)) + ((u32)ctx->fetched_data);
        c = n >= 0x10000;
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {
        z = 0;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
        c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;
    }

    cpu_set_reg(ctx->cur_inst->reg_1, val & 0xFFFF);
    cpu_set_flags(ctx, z, 0, h, c);
}

static void proc_stop(cpu_context *ctx) {
    fprintf(stderr, "STOPPING!\n");
}

static void proc_daa(cpu_context *ctx) {
    u8 reg_value = cpu_read_reg8(RT_A);
    u8 temp = 0;
    u8 lower_nibble = reg_value & 0x0F;
    u8 flag_c = 0;

    if (CPU_FLAG_N) {
        if (CPU_FLAG_H) temp |= 0x06;
        if (CPU_FLAG_C) {
            temp |= 0x60;
            flag_c = 1;
        };
        reg_value -= temp;
    } else {
        if (CPU_FLAG_H || lower_nibble > 0x09) temp |= 0x06;
        if (CPU_FLAG_C || reg_value > 0x99) {
            temp |= 0x60;
            flag_c = 1;
        }
        reg_value += temp;
    }

    cpu_set_reg8(RT_A, reg_value);
    cpu_set_flags(ctx, reg_value == 0, -1, 0, flag_c);
}

static void proc_cpl(cpu_context *ctx) {
    u8 new_val = ~cpu_read_reg8(RT_A);
    cpu_set_reg8(RT_A, new_val);
    cpu_set_flags(ctx, -1, 1, 1, -1);
}

static void proc_scf(cpu_context *ctx) {
    cpu_set_flags(ctx, -1, 0, 0, 1);
}

static void proc_ccf(cpu_context *ctx) {
    cpu_set_flags(ctx, -1, 0, 0, !(CPU_FLAG_C));
}

static void proc_halt(cpu_context *ctx) {
    ctx->halted = true;
}

static void proc_ei(cpu_context *ctx) {
    ctx->enabling_ime = true;
}

static void proc_di(cpu_context *ctx) {
    ctx->int_master_enabled = false;
}



static IN_PROC processors[] = {
    [IN_NOP] = proc_nop,
    [IN_NONE] = proc_none,
    [IN_CB] = proc_cb,
    [IN_RLCA] = proc_rlca,
    [IN_RRCA] = proc_rrca,
    [IN_RLA] = proc_rla,
    [IN_RRA] = proc_rra,
    [IN_AND] = proc_and,
    [IN_XOR] = proc_xor,
    [IN_OR] = proc_or,
    [IN_CP] = proc_cp,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JP] = proc_jp,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_RST] = proc_rst,
    [IN_RET] = proc_ret,
    [IN_RETI] = proc_reti,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push,
    [IN_INC] = proc_inc,
    [IN_DEC] = proc_dec,
    [IN_ADD] = proc_add,
    [IN_ADC] = proc_adc,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    [IN_STOP] = proc_stop,
    [IN_DAA] = proc_daa,
    [IN_CPL] = proc_cpl,
    [IN_SCF] = proc_scf,
    [IN_CCF] = proc_ccf,
    [IN_HALT] = proc_halt,
    [IN_EI] = proc_ei,
    [IN_DI] = proc_di,
};

IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}