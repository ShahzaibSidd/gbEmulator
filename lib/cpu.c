#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpu_context ctx = {0};

void cpu_init() {
    ctx.regs.pc = (u16)0x100;
}

static void fetch_instruction() {
    ctx.cur_opcode = bus_read(ctx.regs.pc++);
    ctx.cur_inst = instruction_by_opcode(ctx.cur_opcode);

    if (ctx.cur_inst == NULL) {
        printf("Uknown Intruction... %02X\n", ctx.cur_opcode);
        exit(-7);
    }
};

static void fetch_data() {
    ctx.mem_dest = 0;
    ctx.dest_is_mem = false;

    switch (ctx.cur_inst->mode) {
        case AM_IMP: return;
        
        case AM_R:
            ctx.fetched_data = cpu_read_reg(ctx.cur_inst->reg_1);
            return;
        
        case AM_R_D8:
            ctx.fetched_data = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;

        case AM_D16:
            u16 low = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;

            u16 high = bus_read(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;

            ctx.fetched_data = (high << 8) | (low);

            return;

        default:
            printf("Unknown Addressing Mode! %d\n", ctx.cur_inst->mode);
            exit(-7);
            return;
    }
};

static void execute() {
    printf("Executing not implemented yet.\n");
};

bool cpu_step() {
   
    if (!ctx.halted) {
        u16 pc = ctx.regs.pc;
        fetch_instruction();
        fetch_data();
        
        printf("Executing Instr %02X    PC: %04X\n", ctx.cur_opcode, pc);
        
        execute();
    }
    if (ctx.regs.pc >0x105) {
        return false;
    }
    return true;
};

