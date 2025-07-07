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

static void proc_none(cpu_context *ctx) {
    printf("INVALID INSTRUCTION OR NOT IMPLEMENTED YET\n");
    exit(-7);
}

static void proc_nop(cpu_context *ctx) {
    //no operation
}

static void proc_ld(cpu_context *ctx) {
    if (ctx->dest_is_mem) {

        //if the register is 16 bit
        if (ctx->cur_inst->reg_2 >= RT_AF) {
            emu_cycles(1);
            bus_write16(ctx->mem_dest, ctx->fetched_data);
        } else {
            bus_write(ctx->mem_dest, ctx->fetched_data);
        }

        return;
    }

    if (ctx->cur_inst->mode == AM_HL_SPR) {
        u8 hflag = (cpu_read_reg((ctx->cur_inst->reg_2) & 0xF) + 
        (ctx->fetched_data & 0xF) >= 0x10); 

        u8 cflag = (cpu_read_reg((ctx->cur_inst->reg_2) & 0xFF) + 
        (ctx->fetched_data & 0xFF) >= 0x100);
        
        cpu_set_flags(ctx, 0, 0, hflag, cflag);

        cpu_set_reg(ctx->cur_inst->reg_1,
                    (cpu_read_reg(ctx->cur_inst->reg_2) + (char)ctx->fetched_data));
    }

    cpu_set_reg(ctx->cur_inst->reg_1, ctx->fetched_data);
    return;
}

static void proc_ldh(cpu_context *ctx) {
    if (ctx->cur_inst->reg_1 == RT_A) {
        cpu_set_reg(ctx->cur_inst->reg_1, bus_read(0xFF00 | ctx->fetched_data));
    } else {
        bus_write(0xFF00 | ctx->mem_dest, ctx->regs.a);
    }
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
        u16 low = stack_pop();
        emu_cycles(1);
        u16 high = stack_pop();
        emu_cycles(1);

        u16 address = low | (high<<8);
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
    u16 val_low = stack_pop();
    emu_cycles(1);
    u16 val_high = stack_pop();

    u16 val = val_low | (val_high << 8);

    cpu_set_reg(ctx->cur_inst->reg_1, val);

    if (ctx->cur_inst->reg_1 == RT_AF) {
        cpu_set_reg(ctx->cur_inst->reg_1, val & 0xFFF0);
    }
}

static void proc_push(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1);
    u16 val_high = (val >> 8) & 0x00FF;
    emu_cycles(1);
    stack_push(val_high);

    u16 val_low = val & 0x00FF; 
    emu_cycles(1);
    stack_push(val_low);

    emu_cycles(1);

    return;
}

static void proc_di(cpu_context *ctx) {
    ctx->int_master_enabled = false;
}

static void proc_xor(cpu_context *ctx) {
    ctx->regs.a ^= ctx->fetched_data &0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}


static IN_PROC processors[] = {
    [IN_NOP] = proc_nop,
    [IN_NONE] = proc_none,
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
    [IN_DI] = proc_di,
    [IN_XOR] = proc_xor,
};

IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}