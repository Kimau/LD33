#pragma once

enum Buttons { B_UP, B_DOWN, B_LEFT, B_RIGHT, NOOF_BUTTONS };
struct SDLAPP;

class GameState {
	int buttonMap[NOOF_BUTTONS];

	// Game State
	SDL_Surface *pGround;
	SDL_Surface *pHouse;
	SDL_Surface *pRoad;

	SDL_Texture *pTexGround;
	SDL_Texture *pTexHouse;
	SDL_Texture *pTexRoad;

	SDL_Rect MapSize;
	SDL_Point camScroll;

	SDLAPP* app;

public:
	void StartGame(SDLAPP *_app);
	void Render();
	void Update();
	void GameEvent(SDL_Event *evt);
};
