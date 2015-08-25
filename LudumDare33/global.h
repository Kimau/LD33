#pragma once

#define SCREEN_TITLE "LudumDare #33 - @EvilKimau"
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 720
#define PIX_WIDTH 320
#define PIX_HEIGHT 240
#define FRAME_RATE 60

#define PIX_MULTI 3

#define PIX_TILE 4
#define PIX_HALF 2

#define COL_ROAD 0x9A9F
#define COL_DIRT 0x953F
#define COL_BEACH 0xFF0F
#define COL_SEA 0x09FF
#define COL_GRASS 0x280F
#define COL_FOREST 0x030F
#define COL_HOUSE 0x000F
#define COL_ROOF 0x420F
#define COL_DOOR 0xFFFF
#define COL_LIGHT 0xFF8F

struct SDLAPP {
  SDL_Window *window;
  SDL_Renderer *renderer;

  SDL_Event LastEvent;
  Uint32 startTime;
  Uint32 frameDelta;
  Uint32 frameStartTime;
  Uint32 frameLock;

};
