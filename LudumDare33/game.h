#pragma once

enum Buttons { B_UP, B_DOWN, B_LEFT, B_RIGHT, NOOF_BUTTONS };
struct SDLAPP;

class GameState {
	int buttonMap[NOOF_BUTTONS];

	SDLAPP* app;

	// Textures
	SDL_Surface *pGround;
	SDL_Surface *pHouse;
	SDL_Surface *pRoad;

	SDL_Texture *pTexGround;
	SDL_Texture *pTexHouse;
	SDL_Texture *pTexRoad;

	// Camera
	SDL_Rect MapSize;
	SDL_Point camScroll;

	// Player
	SDL_Point playerPos;	

	void DrawRect(SDL_Point &p, int w, int h, const SDL_Color& col);

public:
	void StartGame(SDLAPP *_app);
	void Render();
	void Update();
	void GameEvent(SDL_Event *evt);
};
