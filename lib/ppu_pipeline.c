#include <ppu.h>
#include <lcd.h>
#include <bus.h>

void pixel_fifo_push(u32 value) {
    fifo_entry *next = malloc(sizeof(fifo_entry));
    next->next = NULL;
    next->value = value;

    if (!ppu_get_context()->pfc.pixel_fifo.head) {
        //first pixel
        ppu_get_context()->pfc.pixel_fifo.head = ppu_get_context()->pfc.pixel_fifo.tail = next;
    } else {
        ppu_get_context()->pfc.pixel_fifo.tail->next = next;
        ppu_get_context()->pfc.pixel_fifo.tail = next; 
    }

    ppu_get_context()->pfc.pixel_fifo.size++;
}

u32 pixel_fifo_pop() {
    if (!ppu_get_context()->pfc.pixel_fifo.head) {
        fprintf(stderr, "ERROR! POPPING EMPTY PIXEL FIFO\n");
        exit(-8);
    }

    fifo_entry *popped = ppu_get_context()->pfc.pixel_fifo.head;
    ppu_get_context()->pfc.pixel_fifo.head = popped->next;
    
    ppu_get_context()->pfc.pixel_fifo.size--;

    u32 val = popped->value;
    free(popped);
    return val;
}

u32 fetch_sprite_pixels(int bit, u32 colour, u8 bg_colour) {
    for (int i = 0; i < ppu_get_context()->fetched_entry_count; i++) {
        int sp_x = (ppu_get_context()->fetched_entries[i].x - 8) + (lcd_get_context()->scx % 8);
        if (sp_x + 8 < ppu_get_context()->pfc.fifo_x) {
            //past the sprite
            continue;
        }                   
        int offset = ppu_get_context()->pfc.fifo_x - sp_x;
        if (offset < 0 || offset > 7) {
            //before or after the sprite
            continue;
        }

        u8 bit = (7 - offset);
        
        if (ppu_get_context()->fetched_entries[i].f_x_flip) {
            bit = offset;
        }
        
        u8 low = !!(ppu_get_context()->pfc.fetch_entry_data[i * 2] & (1 << bit));
        u8 high = !!((ppu_get_context()->pfc.fetch_entry_data[(i * 2) + 1] & (1 << bit))) << 1;
        bool bg_prio = ppu_get_context()->fetched_entries[i].f_bgp;                 
        if (!(high | low)) {
            continue;
        }                   
        if (!bg_prio || bg_colour == 0) {
            colour = (ppu_get_context()->fetched_entries[i].f_pn) ? lcd_get_context()->sp2_colours[high | low] : lcd_get_context()->sp1_colours[high | low];
            if (high | low) {
                break;
            }
        }
    }

    return colour;
}

bool pipeline_fifo_add() {
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        return false;
    }
    
    int x = ppu_get_context()->pfc.fetch_x - (8 - (lcd_get_context()->scx % 8));

    for (int i = 0; i < 8; i++) {
        int bit = 7 - i;
        u8 high = !!(ppu_get_context()->pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
        u8 low = !!(ppu_get_context()->pfc.bgw_fetch_data[1] & (1 << bit));
        u32 colour = lcd_get_context()->bg_colours[high | low];

        if (!LCDC_BGW_ENABLE) {
            colour = lcd_get_context()->bg_colours[0];
        }

        if (LCDC_OBJ_ENABLE) {
            colour = fetch_sprite_pixels(bit, colour, high | low);
        }

        if (x >= 0) {
            pixel_fifo_push(colour);
            ppu_get_context()->pfc.fifo_x++;
        }
    }

    return true;
}

void pipeline_load_sprite_tile() {
    oam_line_entry *le = ppu_get_context()->line_sprites;

    while (le) {
        int sp_x = (le->entry.x - 8) + (lcd_get_context()->scx % 8);

        if ((sp_x >= ppu_get_context()->pfc.fetch_x && sp_x < ppu_get_context()->pfc.fetch_x + 8) ||
            ((sp_x + 8) >= ppu_get_context()->pfc.fetch_x && (sp_x + 8) < ppu_get_context()->pfc.fetch_x + 8)) {
            
            ppu_get_context()->fetched_entries[ppu_get_context()->fetched_entry_count++] = le->entry;
        }

        le = le->next;

        if (!le || ppu_get_context()->fetched_entry_count >= 3) {
            break;
        }
    }
}

void pipeline_load_sprite_data(u8 data_num) {
    int cur_y = lcd_get_context()->ly;
    u8 sprite_height = LCDC_OBJ_HEIGHT;

    for (int i = 0; i < ppu_get_context()->fetched_entry_count; i++) {
        u8 ty = ((cur_y + 16) - ppu_get_context()->fetched_entries[i].y);

        if (ppu_get_context()->fetched_entries[i].f_y_flip) {
            ty = (sprite_height - 1) - ty;
        }

        u8 index = ppu_get_context()->fetched_entries[i].tile;

        if (sprite_height == 16) {
            //when height is 16, index is always of the top tile, we don't want an odd bit
            index &= ~(1);
        }

        u16 tile_address = 0x8000 + (index << 4);
        u8 data_byte = bus_read(tile_address + (ty * 2) + data_num);
        ppu_get_context()->pfc.fetch_entry_data[(i * 2) + data_num] = data_byte; 
    }
}

void pipeline_fetch() {
    switch (ppu_get_context()->pfc.curr_fetch_state) {
        case FS_TILE: {
            ppu_get_context()->fetched_entry_count = 0;

            if (LCDC_BGW_ENABLE) {
                ppu_get_context()->pfc.bgw_fetch_data[0] = bus_read(LCDC_BG_TILE_MAP + 
                    (ppu_get_context()->pfc.map_x / 8) + ((ppu_get_context()->pfc.map_y / 8) * 32)
                );

                if (LCDC_BGW_TILES == 0x8800) {
                    ppu_get_context()->pfc.bgw_fetch_data[0] += 128;
                }
            }

            if (LCDC_OBJ_ENABLE && ppu_get_context()->line_sprites) {
                pipeline_load_sprite_tile();
            }

            ppu_get_context()->pfc.curr_fetch_state = FS_DATA0;
            ppu_get_context()->pfc.fetch_x += 8;
        } break;
        case FS_DATA0: {
            ppu_get_context()->pfc.bgw_fetch_data[1] = bus_read(LCDC_BGW_TILES +
                (ppu_get_context()->pfc.bgw_fetch_data[0] * 16) +
                ppu_get_context()->pfc.tile_y);

            pipeline_load_sprite_data(0);

            ppu_get_context()->pfc.curr_fetch_state = FS_DATA1;
        } break;
        case FS_DATA1: {
            ppu_get_context()->pfc.bgw_fetch_data[2] = bus_read(LCDC_BGW_TILES +
                (ppu_get_context()->pfc.bgw_fetch_data[0] * 16) +
                ppu_get_context()->pfc.tile_y + 1);
            
            pipeline_load_sprite_data(1);
            
            ppu_get_context()->pfc.curr_fetch_state = FS_IDLE;
        } break;
        case FS_IDLE: {
            ppu_get_context()->pfc.curr_fetch_state = FS_PUSH;
        } break;
        case FS_PUSH: {
            if (pipeline_fifo_add()) {
                ppu_get_context()->pfc.curr_fetch_state = FS_TILE;
            }
        } break;
    }
}

void pipeline_push_pixel() {
    if (ppu_get_context()->pfc.pixel_fifo.size > 8) {
        u32 pixel_data = pixel_fifo_pop();

        if (ppu_get_context()->pfc.line_x >= (lcd_get_context()->scx % 8)) {
            ppu_get_context()->video_buffer[ ppu_get_context()->pfc.pushed_x + (lcd_get_context()->ly * XRES) ] = pixel_data;
            
            ppu_get_context()->pfc.pushed_x++;
        }

        ppu_get_context()->pfc.line_x++;
    }
}

void pipeline_process() {
    ppu_get_context()->pfc.map_y = (lcd_get_context()->ly + lcd_get_context()->scy);
    ppu_get_context()->pfc.map_x = (ppu_get_context()->pfc.fetch_x + lcd_get_context()->scx);
    ppu_get_context()->pfc.tile_y = ((lcd_get_context()->ly + lcd_get_context()->scy) % 8) * 2;

    if (!(ppu_get_context()->line_ticks & 1)) {
        pipeline_fetch();
    }

    pipeline_push_pixel();
}

void pipeline_fifo_reset() {
    while (ppu_get_context()->pfc.pixel_fifo.size) {
        pixel_fifo_pop();
    }
    ppu_get_context()->pfc.pixel_fifo.head = 0;
}