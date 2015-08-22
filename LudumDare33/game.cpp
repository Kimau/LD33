#include "SDL.h"
#include "global.h"
#include "sprite.h"
#include "game.h"

#define PIX_MULTI 6

void GameState::StartGame(SDLAPP *_app) {
  app = _app;

  pGround = LoadBMP("ground.bmp");
  pHouse = LoadBMP("house.bmp");
  pRoad = LoadBMP("road.bmp");

  SDL_SetColorKey(pHouse, SDL_TRUE, ((Uint16 *)pHouse->pixels)[0]);
  SDL_SetColorKey(pRoad, SDL_TRUE, ((Uint16 *)pRoad->pixels)[0]);

  pTexGround = SDL_CreateTextureFromSurface(app->renderer, pGround);
  pTexHouse = SDL_CreateTextureFromSurface(app->renderer, pHouse);
  pTexRoad = SDL_CreateTextureFromSurface(app->renderer, pRoad);

  MapSize = pGround->clip_rect;
  camScroll = SDL_Point{0, 0};
}

void GameState::Render() {
  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
  SDL_RenderClear(app->renderer);

  // TODO :: Render
  SDL_SetRenderDrawColor(app->renderer, 200, 0, 0, 255);
  SDL_RenderDrawLine(app->renderer, 10, 10, 200, 200);

  SDL_Rect tarRect =
      SDL_Rect{-camScroll.x * PIX_MULTI, -camScroll.y * PIX_MULTI,
               MapSize.w * PIX_MULTI, MapSize.h * PIX_MULTI};

  SDL_RenderCopy(app->renderer, pTexGround, &pGround->clip_rect, &tarRect);
  SDL_RenderCopy(app->renderer, pTexRoad, &pGround->clip_rect, &tarRect);
  SDL_RenderCopy(app->renderer, pTexHouse, &pGround->clip_rect, &tarRect);

  SDL_RenderPresent(app->renderer);
}

void GameState::Update() {
  // Debug Camera Scroll
  camScroll.x +=
      ((buttonMap[B_LEFT] > 0) ? -1 : 0) + ((buttonMap[B_RIGHT] > 0) ? +1 : 0);
  camScroll.y +=
      ((buttonMap[B_UP] > 0) ? -1 : 0) + ((buttonMap[B_DOWN] > 0) ? +1 : 0);

  if (camScroll.x < 0) camScroll.x = 0;
  if (camScroll.y < 0) camScroll.y = 0;
  if (camScroll.x > MapSize.w - app->width / PIX_MULTI)
    camScroll.x = MapSize.w - app->width / PIX_MULTI;
  if (camScroll.y > MapSize.h - app->height / PIX_MULTI)
    camScroll.y = MapSize.h - app->height / PIX_MULTI;

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
