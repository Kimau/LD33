#pragma once

#define SCREEN_TITLE "LudumDare #33 - @EvilKimau"
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 720
#define PIX_WIDTH 320
#define PIX_HEIGHT 240
#define FRAME_RATE 60

#define PIX_MULTI 3

#define PIX_TILE 5
#define PIX_HALF 2



struct SDLAPP {
  SDL_Window *window;
  SDL_Renderer *renderer;

  SDL_Event LastEvent;
  Uint32 startTime;
  Uint32 frameDelta;
  Uint32 frameStartTime;
  Uint32 frameLock;

};
