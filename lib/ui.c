#include <ui.h>
#include <emu.h>
#include <bus.h>
#include <ppu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

SDL_Window *sdlDebugWindow;
SDL_Renderer *sdlDebugRenderer;
SDL_Texture *sdlDebugTexture;
SDL_Surface *debugScreen;

int scale = 4;

void ui_init() {
    
    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow, &sdlRenderer);
    
    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   SCREEN_WIDTH, SCREEN_HEIGHT);

    //--------

    SDL_CreateWindowAndRenderer(16 * 8 * scale, 32 * 8 * scale, 0, &sdlDebugWindow, &sdlDebugRenderer);

    debugScreen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale), 32,
                                        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    sdlDebugTexture = SDL_CreateTexture(sdlDebugRenderer,
                                        SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        (16 * 8 * scale) + (16 * scale),
                                        (32 * 8 * scale) + (64 * scale));

    int x, y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow, x + SCREEN_WIDTH + 10, y);
}

void delay(u32 ms) {
    SDL_Delay(ms);
}

u32 get_ticks() {
    return SDL_GetTicks(); 
}

static unsigned long tile_colours[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void draw_tile(SDL_Surface *surface, u16 address, u16 tileNum, int x, int y) {
    SDL_Rect rc;
    rc.h = scale;
    rc.w = scale;
    
    for (u8 lineNum = 0; lineNum < 8; lineNum++) {
        u8 b1 = bus_read(address + (tileNum * 16));
        u8 b2 = bus_read(address + (tileNum * 16) + 1);

        for (u8 pixelNum = 0; pixelNum < 8; pixelNum++) {
            u8 large_bit = ((b2 << pixelNum >> 7) & 1) << 1;
            u8 small_bit = ((b1 << pixelNum >> 7) & 1);
            unsigned long colour = tile_colours[large_bit | small_bit];

            rc.x = x;
            rc.y = y;

            SDL_FillRect(debugScreen, &rc, colour);
            x += scale;
        }

        address += 2;
        y += scale;
        x -= (scale * 8);
    }
}

void update_debug_window() {
    int xDraw = 0;
    int yDraw = 0;
    int tileNum = 0;

    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = debugScreen->w;
    rc.h = debugScreen->h;

    SDL_FillRect(debugScreen, &rc, 0xFF111111);

    u16 addr = 0x8000;
    //384 total tiles in VRAM, so a 24 x 16 grid of tiles
    for (u8 y = 0; y < 24; y++) {
        for (u8 x = 0; x < 16; x++) {
            draw_tile(debugScreen, addr, tileNum, xDraw, yDraw);
            xDraw += (8 * scale) + (scale);
            tileNum++;
        }

        yDraw += (8 * scale) + (scale);
        xDraw = 0;
    } 

    SDL_UpdateTexture(sdlDebugTexture, NULL, debugScreen->pixels, debugScreen->pitch);
    SDL_RenderClear(sdlDebugRenderer);
    SDL_RenderCopy(sdlDebugRenderer, sdlDebugTexture, NULL, NULL);
    SDL_RenderPresent(sdlDebugRenderer);
}

void ui_update() {
    SDL_Rect rc;
    rc.x = rc.y = 0;
    rc.w = rc.h = 2048;

    u32 *video_buffer = ppu_get_context()->video_buffer;

    for (int line_num = 0; line_num < YRES; line_num++) {
        for (int x = 0; x < XRES; x++) {
            rc.x = x * scale;
            rc.y = line_num * scale;
            rc.h = scale;
            rc.w = scale;

            SDL_FillRect(screen, &rc, video_buffer[x + (line_num * XRES)]);
        }
    }

    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);

    update_debug_window();
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0)
    {
        //TODO SDL_UpdateWindowSurface(sdlWindow);
        //TODO SDL_UpdateWindowSurface(sdlTraceWindow);
        //TODO SDL_UpdateWindowSurface(sdlDebugWindow);

        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            emu_get_context()->die = true;
        }
    }
}