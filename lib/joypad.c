#include <joypad.h>
#include <string.h>

static joypad_context ctx = {0};

joypad_state *joypad_get_state() {
    return &ctx.jp_state;
}

bool joypad_button_sel() {
    return ctx.button_sel;
}

bool joypad_dpad_sel() {
    return ctx.dpad_sel;
}

void joypad_set_sel(u8 val) {
    ctx.button_sel = val & 0x20;
    ctx.dpad_sel = val & 0x10;
}

u8 joypad_get_output() {
    u8 output = 0b11001111;
    joypad_state *state = joypad_get_state();

    if (!joypad_button_sel()) {
        if (state->start) {
            output &= ~(1 << 3);
        }
        if (state->select) {
            output &= ~(1 << 2);
        }
        if (state->b) {
            output &= ~(1 << 1);
        }
        if (state->a) {
            output &= ~(1);
        }
    }

    if (!joypad_dpad_sel()) {
        if (state->down) {
            output &= ~(1 << 3);
        }
        if (state->up) {
            output &= ~(1 << 2);
        }
        if (state->left) {
            output &= ~(1 << 1);
        }
        if (state->right) {
            output &= ~(1);
        }
    }
    
    return output;
}