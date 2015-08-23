#include "SDL.h"
#include "SDL2_gfxPrimitives.h"
#include "global.h"
#include "sprite.h"
#include "game.h"


struct House {
  SDL_Point topLeft;
  SDL_Rect bounds;
  SDL_Texture *houseTex;
  SDL_Point *points;
  int numWallPts;
  int totalPts;
  bool isRender;
};
static std::list<House> s_houses;

void BuildHouse(SDL_Renderer *pRender,
                std::list<std::list<HousePoint>>::reference &oh) {
  House h;
  h.totalPts = oh.size() + 1;
  h.numWallPts = 0;
  int doorCurr = h.totalPts-1;
  h.points = new SDL_Point[h.totalPts];
  h.bounds = SDL_Rect{100000, 1000000, 0, 0};
  for (auto p : oh) {
	  SDL_Point pt = SDL_Point{ p.x * PIX_TILE + PIX_HALF, p.y * PIX_TILE + PIX_HALF };

    if (pt.x < h.bounds.x) h.bounds.x = pt.x;
    if (pt.y < h.bounds.y) h.bounds.y = pt.y;
    if (pt.x > h.bounds.w) h.bounds.w = pt.x;
    if (pt.y > h.bounds.h) h.bounds.h = pt.y;

	if (p.isDoor)
		h.points[doorCurr--] = pt;
	else
		h.points[h.numWallPts++] = pt;
  }
  h.points[h.numWallPts++] = h.points[0];

  h.topLeft.x = h.bounds.x - 8;
  h.topLeft.y = h.bounds.y - 8;
  h.bounds = SDL_Rect{0, 0, h.bounds.w - h.topLeft.x + 16,
                      h.bounds.h - h.topLeft.y + 16};

  for (int i = 0; i < h.totalPts; ++i) {
    h.points[i].x = h.points[i].x - h.topLeft.x;
    h.points[i].y = h.points[i].y - h.topLeft.y;
  }

  h.houseTex =
      SDL_CreateTexture(pRender, SDL_PIXELFORMAT_RGBA4444,
                        SDL_TEXTUREACCESS_TARGET, h.bounds.w, h.bounds.h);
  SDL_SetTextureBlendMode(h.houseTex, SDL_BLENDMODE_BLEND);

  // Update House Tex
  SDL_SetRenderTarget(pRender, h.houseTex);
  SDL_SetRenderDrawColor(pRender, 0, 0, 0, 0);
  SDL_RenderClear(pRender);

  int16_t* vx = new int16_t[h.numWallPts];
  int16_t* vy = new int16_t[h.numWallPts];
  for (int i = 0; i < h.numWallPts; ++i) {
	  vx[i] = h.points[i].x;
	  vy[i] = h.points[i].y;
  }
  filledPolygonRGBA(pRender, vx, vy, h.numWallPts, 60,60,60, 255);
  polygonRGBA(pRender, vx, vy, h.numWallPts, 0, 0, 0, 128);
  delete vx, vy;

  for (int i = h.numWallPts+1; i < h.totalPts; i+=2) {
	  thickLineRGBA(pRender, h.points[i - 1].x, h.points[i - 1].y, h.points[i].x, h.points[i].y, 3, 255, 0, 255, 255);
  }

  SDL_RenderPresent(pRender);
  SDL_SetRenderTarget(pRender, NULL);
  h.isRender = true;

  //
  s_houses.push_back(h);
}

//////////////////////////////////////////////////////////////////////////
// Useful Small Functions

void GameState::DrawRect(SDL_Point &p, int w, int h, const SDL_Color &col) {
  SDL_SetRenderDrawColor(app->renderer, col.r, col.g, col.b, col.a);
  SDL_RenderFillRect(app->renderer, &SDL_Rect{p.x - camScroll.x - w/2,
                                              p.y - camScroll.y - h/2, w, h});

  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 128);
  SDL_RenderDrawRect(app->renderer, &SDL_Rect{p.x - camScroll.x - w/2,
                                              p.y - camScroll.y - h/2, w, h});
}

//////////////////////////////////////////////////////////////////////////
void GameState::StartGame(SDLAPP *_app) {
  app = _app;

  pGround = LoadBMP("ground.bmp");
  pHouse = LoadBMP("house.bmp");
  pGrad = LoadBMP("grad.bmp");

  // Player
  MapSize = pGround->clip_rect;
  camScroll = SDL_Point{ 0, 0 };
  playerPos = SDL_Point{ 0, 0 };

  // Setup Houses
  s_houses.clear();
  SDL_LockSurface(pHouse);
  auto housesList = ExtractHouses((uint16_t *)pHouse->pixels,
                                  pHouse->clip_rect.w, pHouse->clip_rect.h);
  SDL_UnlockSurface(pHouse);

  while (housesList.empty() == false) {
    auto oh = housesList.front();
    BuildHouse(app->renderer, oh);
    housesList.pop_front();
  }

  // Setup Ground
  pTexLevel = SDL_CreateTextureFromSurface(app->renderer, pGround);
}

void GameState::Render() {
	SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
	SDL_RenderClear(app->renderer);

	SDL_Rect tarRect = SDL_Rect{ -camScroll.x, -camScroll.y, MapSize.w * PIX_TILE,
		MapSize.h * PIX_TILE };

	SDL_RenderCopy(app->renderer, pTexLevel, &pGround->clip_rect, &tarRect);
	/*
	for (auto h : s_houses) {
		SDL_Rect r = h.bounds;
		r.x = h.topLeft.x - camScroll.x;
		r.y = h.topLeft.y - camScroll.y;
		SDL_RenderCopy(app->renderer, h.houseTex, &h.bounds, &r);
	}*/

	DrawRect(playerPos, 6, 6, SDL_Color{ 200, 0, 0, 255 });

	SDL_RenderPresent(app->renderer);
}

void GameState::Update() {
  // Debug Camera Scroll
  camScroll.x += ((buttonMap[B_LEFT] > 0) ? -PIX_TILE : 0) +
                 ((buttonMap[B_RIGHT] > 0) ? +PIX_TILE : 0);
  camScroll.y += ((buttonMap[B_UP] > 0) ? -PIX_TILE : 0) +
                 ((buttonMap[B_DOWN] > 0) ? +PIX_TILE : 0);

  if (camScroll.x < 0) camScroll.x = 0;
  if (camScroll.y < 0) camScroll.y = 0;
  if (camScroll.x > MapSize.w*PIX_TILE - PIX_WIDTH)
	  camScroll.x = MapSize.w*PIX_TILE - PIX_WIDTH;
  if (camScroll.y > MapSize.h*PIX_TILE - PIX_HEIGHT)
	  camScroll.y = MapSize.h*PIX_TILE - PIX_HEIGHT;

  // Clear Buttons
  for (int b = NOOF_BUTTONS - 1; b >= 0; --b) {
    buttonMap[b] &= 1;
  }
}

void GameState::GameEvent(SDL_Event *evt) {
  switch (evt->type) {
    case SDL_KEYDOWN:
      switch (evt->key.keysym.scancode) {
        case SDL_SCANCODE_UP:
          buttonMap[B_UP] = 3;
          break;
        case SDL_SCANCODE_DOWN:
          buttonMap[B_DOWN] = 3;
          break;
        case SDL_SCANCODE_LEFT:
          buttonMap[B_LEFT] = 3;
          break;
        case SDL_SCANCODE_RIGHT:
          buttonMap[B_RIGHT] = 3;
          break;
      }
      break;

    case SDL_KEYUP:
      switch (evt->key.keysym.scancode) {
        case SDL_SCANCODE_UP:
          buttonMap[B_UP] = 0;
          break;
        case SDL_SCANCODE_DOWN:
          buttonMap[B_DOWN] = 0;
          break;
        case SDL_SCANCODE_LEFT:
          buttonMap[B_LEFT] = 0;
          break;
        case SDL_SCANCODE_RIGHT:
          buttonMap[B_RIGHT] = 0;
          break;
      }
      break;

    case SDL_MOUSEMOTION:
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
	{
		
		auto pt = SDL_Point{ 
			(evt->button.x / PIX_MULTI) + camScroll.x,
			(evt->button.y / PIX_MULTI) + camScroll.y };

		playerPos = pt;

		SDL_Log("Mouse %d,%d", pt.x, pt.y);
		for (auto h : s_houses) {

		}
	}
	
    case SDL_MOUSEWHEEL:  break;
  }
}

//
