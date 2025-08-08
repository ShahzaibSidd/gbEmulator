#pragma once

#include <common.h>

typedef struct {
    //registers
    u8 lcd_control;     //FF40
    u8 status;          //FF41
    u8 scx;             //FF42
    u8 scy;             //FF43
    u8 ly;              //FF44
    u8 lyc;             //FF45
    u8 dma;             //FF46
    u8 bg_palette;      //FF47
    u8 obj_palette[2];  //FF48-FF49
    u8 win_y;           //FF4A
    u8 win_x;           //FF4B

    u32 bg_colours[4];
    u32 sp1_colours[4];
    u32 sp2_colours[4];
} lcd_context;

typedef enum {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_XFER,
} lcd_mode;

lcd_context *lcd_get_context();

#define LCDC_BGW_ENABLE (BIT(lcd_get_context()->lcd_control, 0))
#define LCDC_OBJ_ENABLE (BIT(lcd_get_context()->lcd_control, 1))
#define LCDC_OBJ_HEIGHT (BIT(lcd_get_context()->lcd_control, 2) ? 16 : 8)
#define LCDC_BG_TILE_MAP (BIT(lcd_get_context()->lcd_control, 3) ? 0x9C00 : 0x9800)
#define LCDC_BGW_TILES (BIT(lcd_get_context()->lcd_control, 4) ? 0x8000 : 0x8800)
#define LCDC_WINDOW_ENABLE (BIT(lcd_get_context()->lcd_control, 5))
#define LCDC_WINDOW_TILE_MAP (BIT(lcd_get_context()->lcd_control, 6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_PPU_ENABLE (BIT(lcd_get_context()->lcd_control, 7))

#define STATUS_MODE ((lcd_mode)(lcd_get_context()->status & 0b11))
#define STATUS_MODE_SET(mode) { lcd_get_context()->status &= ~0b11; lcd_get_context()->status |= mode; }

#define STATUS_LYC (BIT(lcd_get_context()->status, 2))
#define STATUS_LYC_SET(b) (BIT_SET(lcd_get_context()->status, 2, b))

typedef enum {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
} stat_src;

#define STATUS_INT(src) (lcd_get_context()->status & src)

void lcd_init();

u8 lcd_read(u16 address);
void lcd_write(u16 address, u8 value);