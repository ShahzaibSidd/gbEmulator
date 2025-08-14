#pragma once

#include <common.h>

typedef struct {
    bool start;
    bool select;
    bool b;
    bool a;
    bool up;
    bool down;
    bool left;
    bool right;
} joypad_state;

typedef struct {
    bool button_sel;
    bool dpad_sel;
    joypad_state jp_state;
} joypad_context;

void joypad_init();
void joypad_set_sel(u8 val);

bool joypad_button_sel();
bool joypad_dpad_sel();

joypad_state *joypad_get_state();
u8 joypad_get_output();