#include <ppu.h>
#include <lcd.h>
#include <cpu.h>
#include <interrupts.h>

void increment_ly() {
    lcd_get_context()->ly++;

    if (lcd_get_context()->ly == lcd_get_context()->lyc) {
        STATUS_LYC_SET(1);

        if (STATUS_INT(SS_LYC)) {
            cpu_request_interrupt(IT_LCD_STAT);
        }
    } else {
        STATUS_LYC_SET(0);
    }
}

void ppu_mode_oam() {
    if (ppu_get_context()->line_ticks >= 80) {
        STATUS_MODE_SET(MODE_XFER);

        ppu_get_context()->pfc.curr_fetch_state = FS_TILE;
        ppu_get_context()->pfc.line_x = 0;
        ppu_get_context()->pfc.fetch_x = 0;
        ppu_get_context()->pfc.pushed_x = 0;
        ppu_get_context()->pfc.fifo_x = 0;
    }
}

void ppu_mode_xfer() {
    pipeline_process();

    if (ppu_get_context()->pfc.pushed_x >= XRES) {
        pipeline_fifo_reset();

        STATUS_MODE_SET(MODE_HBLANK);

        if (STATUS_INT(SS_HBLANK)) {
            cpu_request_interrupt(IT_LCD_STAT);
        }
    }
}

void ppu_mode_vblank() {
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE) {
        increment_ly();
        
        if (lcd_get_context()->ly >= LINES_PER_FRAME) {
            STATUS_MODE_SET(MODE_OAM);
            lcd_get_context()->ly = 0;
        }

        ppu_get_context()->line_ticks = 0;
    }
}

static u32 target_frame_time = 1000 / 60;
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;

void ppu_mode_hblank() {
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        if (lcd_get_context()->ly >= YRES) {
            STATUS_MODE_SET(MODE_VBLANK);

            cpu_request_interrupt(IT_VBLANK);

            if (STATUS_INT(SS_VBLANK)) {
                cpu_request_interrupt(IT_LCD_STAT);
            }

            ppu_get_context()->current_frame++;

            //calculating fps
            u32 end = get_ticks();
            u32 frame_time = end - prev_frame_time;

            if (frame_time < target_frame_time) {
                delay(target_frame_time - frame_time);
            }

            if (end - start_timer >= 1000) {
                u32 fps = frame_count;
                start_timer = end;
                frame_count = 0;

                printf("FPS: %d\n", fps);
            }

            frame_count++;
            prev_frame_time = get_ticks();

        } else {
            STATUS_MODE_SET(MODE_OAM);
        }

        ppu_get_context()->line_ticks = 0;
    }
}