#pragma once

#define SCREEN_TITLE "LudumDare #33 - @EvilKimau"
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FRAME_RATE 60

struct SDLAPP {
	SDL_Window *window;
	SDL_Renderer *renderer;

	SDL_Event LastEvent;
	Uint32 startTime;
	Uint32 frameDelta;
	Uint32 frameStartTime;
	Uint32 frameLock;

	int width;
	int height;
};

