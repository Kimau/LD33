#pragma once

enum Buttons { B_UP, B_DOWN, B_LEFT, B_RIGHT, 
	B_CAMUP, B_CAMDOWN, B_CAMLEFT, B_CAMRIGHT, NOOF_BUTTONS
};
struct SDLAPP;

class GameState {
  int buttonMap[NOOF_BUTTONS];

  SDLAPP *app;

  // Textures
  SDL_Surface *pGround;
  SDL_Surface *pHouse;
  SDL_Surface *pGrad;


  SDL_Texture *pTexLevel;
  SDL_Texture *pTexLight;

  // Camera
  SDL_Rect MapSize;
  SDL_Point camScroll;

  // Player
  SDL_Point playerPos;

  void DrawGridRect(SDL_Point &p, int w, int h, const SDL_Color &col);
  void BoundToMap(SDL_Point& pt, int w, int h) const;

  int GetTiles(const SDL_Point &pt, int* fourInts) const;
  bool UpdateMovePlayer(SDL_Point newPos);

 public:
  void StartGame(SDLAPP *_app);
  void Render();
  void Update();

  void LightSweep(uint32_t* pix);

  void UpdateCamera(SDL_Point &playVel);

  void GameEvent(SDL_Event *evt);
};
