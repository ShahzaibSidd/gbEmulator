#pragma once
#include <cpu.h>

typedef enum {
    IT_VBLANK   = 0b00000001,
    IT_LCD_STAT = 0b00000010,
    IT_TIMER    = 0b00000100,
    IT_SERIAL   = 0b00001000,
    IT_JOYPAD   = 0b00010000
} interrupt_type;

void cpu_request_interrupt(interrupt_type it);

void cpu_handle_interrupt(cpu_context *ctx);
