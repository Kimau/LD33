#include "SDL.h"
#include "global.h"
#include "sprite.h"
#include "game.h"

#define PIX_MULTI 6



struct House
{
	SDL_Point topLeft;
	SDL_Rect bounds;
	SDL_Texture* houseTex;
	SDL_Point* points;
	int numPts;
};
static std::list <House> s_houses;


void BuildHouse(SDL_Renderer* pRender, std::list<std::list<SDL_Point>>::reference &oh)
{
	House h;
	h.numPts = 0;
	h.points = new SDL_Point[oh.size()+1];
	h.bounds = SDL_Rect{ 100000, 1000000, 0, 0 };
	for (auto p : oh)
	{
		SDL_Point pt = SDL_Point{ p.x*PIX_MULTI + 3, p.y * PIX_MULTI + 3 };

		if (pt.x < h.bounds.x) h.bounds.x = pt.x;
		if (pt.y < h.bounds.y) h.bounds.y = pt.y;
		if (pt.x > h.bounds.w) h.bounds.w = pt.x;
		if (pt.y > h.bounds.h) h.bounds.h = pt.y;

		h.points[h.numPts++] = pt;
	}
	h.points[h.numPts++] = h.points[0];

	h.topLeft.x = h.bounds.x-8;
	h.topLeft.y = h.bounds.y-8;
        h.bounds = SDL_Rect{0, 0, h.bounds.w - h.topLeft.x + 16,
                            h.bounds.h - h.topLeft.y + 16};

        for (int i = 0; i < h.numPts; ++i) {
		h.points[i].x = h.points[i].x - h.topLeft.x;
		h.points[i].y = h.points[i].y - h.topLeft.y;
	}

	h.houseTex = SDL_CreateTexture(pRender, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_TARGET, h.bounds.w, h.bounds.h);
	SDL_SetTextureBlendMode(h.houseTex, SDL_BLENDMODE_BLEND);

	// Update House Tex
	SDL_SetRenderTarget(pRender, h.houseTex);
	SDL_SetRenderDrawColor(pRender, 0, 0, 0, 0);
	SDL_RenderClear(pRender);


	SDL_SetRenderDrawColor(pRender, 0, 255, 0, 255);
	for (int i = 0; i < h.numPts; ++i) {
		SDL_RenderFillRect(pRender, &SDL_Rect{ h.points[i].x - 2, h.points[i].y - 2, 4, 4 });
	}


	SDL_SetRenderDrawColor(pRender, 0, 0, 0, 255);
	SDL_RenderDrawLines(pRender, h.points, h.numPts);


	SDL_RenderPresent(pRender);

	SDL_SetRenderTarget(pRender, NULL);

	//
	s_houses.push_back(h);
}

//////////////////////////////////////////////////////////////////////////
// Useful Small Functions

void GameState::DrawRect(SDL_Point &p, int w, int h, const SDL_Color &col) {
	SDL_SetRenderDrawColor(app->renderer, col.r, col.g, col.b, col.a);
	SDL_RenderFillRect(app->renderer, &SDL_Rect{ p.x - camScroll.x - w,
		p.y - camScroll.y - h, w, h });

	SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 128);
	SDL_RenderDrawRect(app->renderer, &SDL_Rect{ p.x - camScroll.x - w,
		p.y - camScroll.y - h, w, h });
}

//////////////////////////////////////////////////////////////////////////
void GameState::StartGame(SDLAPP *_app) {
  app = _app;
  
  pGround = LoadBMP("ground.bmp");
  pHouse = LoadBMP("house.bmp");
  pRoad = LoadBMP("road.bmp");

  // Setup Houses
  s_houses.clear();
  SDL_LockSurface(pHouse);
  auto housesList = ExtractHouses((uint16_t *)pHouse->pixels, pHouse->clip_rect.w,
                pHouse->clip_rect.h);
  SDL_UnlockSurface(pHouse);


  while (housesList.empty() == false) {
	  auto oh = housesList.front();
	  BuildHouse(app->renderer, oh);
	  housesList.pop_front();
  }


  // Setup Textures
  SDL_SetColorKey(pRoad, SDL_TRUE, ((Uint16 *)pRoad->pixels)[0]);
  pTexGround = SDL_CreateTextureFromSurface(app->renderer, pGround);
  pTexHouse = SDL_CreateTextureFromSurface(app->renderer, pHouse);
  SDL_SetTextureBlendMode(pTexHouse, SDL_BLENDMODE_BLEND);
  pTexRoad = SDL_CreateTextureFromSurface(app->renderer, pRoad);

  MapSize = pGround->clip_rect;
  camScroll = SDL_Point{0, 0};

  // Player

  playerPos = SDL_Point{100, 200};
}


void GameState::Render() {
  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
  SDL_RenderClear(app->renderer);

  // TODO :: Render
  SDL_SetRenderDrawColor(app->renderer, 200, 0, 0, 255);
  SDL_RenderDrawLine(app->renderer, 10, 10, 200, 200);

  SDL_Rect tarRect = SDL_Rect{-camScroll.x, -camScroll.y, MapSize.w * PIX_MULTI,
                              MapSize.h * PIX_MULTI};

  SDL_RenderCopy(app->renderer, pTexGround, &pGround->clip_rect, &tarRect);
  SDL_RenderCopy(app->renderer, pTexRoad, &pGround->clip_rect, &tarRect);
  //SDL_RenderCopy(app->renderer, pTexHouse, &pGround->clip_rect, &tarRect);

  for (auto h : s_houses)
  {
	  SDL_Rect r = h.bounds;
	  r.x = h.topLeft.x - camScroll.x;
	  r.y = h.topLeft.y - camScroll.y;
	  SDL_RenderCopy(app->renderer, h.houseTex, &h.bounds, &r);
  }

  DrawRect(playerPos, 14, 14, SDL_Color{200, 0, 0, 255});

  SDL_RenderPresent(app->renderer);
}

void GameState::Update() {
  // Debug Camera Scroll
  camScroll.x += ((buttonMap[B_LEFT] > 0) ? -10 : 0) +
                 ((buttonMap[B_RIGHT] > 0) ? +10 : 0);
  camScroll.y +=
      ((buttonMap[B_UP] > 0) ? -10 : 0) + ((buttonMap[B_DOWN] > 0) ? +10 : 0);

  if (camScroll.x < 0) camScroll.x = 0;
  if (camScroll.y < 0) camScroll.y = 0;
  if (camScroll.x > (MapSize.w * PIX_MULTI) - app->width)
    camScroll.x = (MapSize.w * PIX_MULTI) - app->width;
  if (camScroll.y > (MapSize.h * PIX_MULTI) - app->height)
    camScroll.y = (MapSize.h * PIX_MULTI) - app->height;

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
    case SDL_MOUSEWHEEL: {
      auto pt = SDL_Point{evt->button.x, evt->button.y};
      SDL_Log("Mouse %d,%d", pt.x, pt.y);
    } break;
  }
}

//
