#include <cpu.h>
#include <stack.h>
#include <interrupts.h>

void int_handle(cpu_context *ctx, u16 address) {
    stack_push16(ctx->regs.pc);
    ctx->regs.pc = address;
}

bool int_check(cpu_context *ctx, u16 address, interrupt_type it) {
    if (ctx->int_flags & it && ctx->ie_register & it) {
        int_handle(ctx, address);
        ctx->int_flags &= ~(it);
        ctx->halted = false;
        ctx->int_master_enabled = false;

        return true;
    }

    return false;
}

void cpu_handle_intterupt(cpu_context *ctx) {
    if (int_check(ctx, 0x0040, IT_VBLANK)) {
        return;
    } else if (int_check(ctx, 0x0048, IT_LCD_STAT)) {
        return;
    } else if (int_check(ctx, 0x0050, IT_TIMER)) {
        return;
    } else if (int_check(ctx, 0x0058, IT_SERIAL)) {
        return;
    } else if (int_check(ctx, 0x0060, IT_JOYPAD)) {
        return;
    }
}