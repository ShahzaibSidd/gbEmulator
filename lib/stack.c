#include <stack.h>
#include <cpu.h>
#include <bus.h>

void stack_push(u8 data) {
    cpu_get_regs()->sp--;
    bus_write(cpu_get_regs()->sp, data);
    return;
}

void stack_push16(u16 data) {
    u8 low = data & 0x00FF;
    u8 high = data >> 8 & 0x00FF;

    stack_push(high);
    stack_push(low);
    return;
}

u8 stack_pop() {
    u8 val = bus_read(cpu_get_regs()->sp++);
    return val;
}

u16 stack_pop16() {
    u16 val_low = stack_pop();
    u16 val_high = stack_pop();

    return (val_low | (val_high << 8));
}