#include "SDL.h"
#include "game.h"
#include "global.h"

bool bShowGrid = false;

SDLAPP *CreateApp() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "SDL_Init");
    return nullptr;
  }

  SDLAPP *pApp = new SDLAPP();
  
  pApp->startTime = SDL_GetTicks();
  pApp->frameLock = 1000 / FRAME_RATE;

  pApp->window = SDL_CreateWindow(SCREEN_TITLE, 100, 100, SCREEN_WIDTH,
                                  SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (pApp->window == nullptr) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateWindow");
    delete pApp;
    return nullptr;
  }

  pApp->renderer = SDL_CreateRenderer(
      pApp->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (pApp->renderer == nullptr) {
    SDL_LogError(SDL_LOG_CATEGORY_RENDER, "SDL_CreateRenderer");
    SDL_DestroyWindow(pApp->window);
    delete pApp;
    return nullptr;
  }

  return pApp;
}

void CleanQuit(SDLAPP *pApp) {
  SDL_DestroyRenderer(pApp->renderer);
  SDL_DestroyWindow(pApp->window);
  SDL_Quit();
}

int GameEventStep(SDLAPP *pApp, GameState *pGame) {
  // Events
  auto evt = &pApp->LastEvent;
  while (SDL_PollEvent(evt)) {
    switch (evt->type) {
      case SDL_WINDOWEVENT:
        // if (event.window.windowID == windowID)
        switch (evt->window.event) {
          case SDL_WINDOWEVENT_CLOSE: {
            SDL_Log("Event: Close Window");
            break;
          }
        }
        break;


	  case SDL_KEYUP:
		  switch (evt->key.keysym.scancode) {
		  case SDL_SCANCODE_G:
			  bShowGrid = !bShowGrid;
		  };
		  pGame->GameEvent(evt);
		  break;

      case SDL_QUIT:
        SDL_Log("QUIT");
        return 0;

      default:
        // Handle Event in Game
        pGame->GameEvent(evt);
    }
  }

  return 1;
}

int main(int argc, char *argv[]) {
  SDLAPP *pApp = CreateApp();
  GameState Game;
  Game.StartGame(pApp);

  if (pApp == nullptr) {
    return -1;
  }

  if (GameEventStep(pApp, &Game) == 0) {
    CleanQuit(pApp);
    return 0;
  }

  SDL_Texture* pixTar = SDL_CreateTexture(pApp->renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, PIX_WIDTH, PIX_HEIGHT);
  SDL_Rect srcRect = { 0, 0, PIX_WIDTH, PIX_HEIGHT };
  SDL_Rect destRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
  
  do {
	  int newTime = SDL_GetTicks();
	  pApp->frameDelta = newTime - pApp->frameStartTime;
	  pApp->frameStartTime = newTime;

	  Game.Update();

	  SDL_SetRenderTarget(pApp->renderer, pixTar);
	  Game.Render();
	  SDL_SetRenderTarget(pApp->renderer, NULL);
	  SDL_RenderCopy(pApp->renderer, pixTar, &srcRect, &destRect);

	  if (bShowGrid) {
			SDL_SetRenderDrawColor(pApp->renderer, 0, 0, 0, 255);
			for (int x = 0; x < SCREEN_WIDTH; x += PIX_MULTI*PIX_TILE)
				SDL_RenderDrawLine(pApp->renderer, x, 0, x, SCREEN_HEIGHT);

			for (int y = 0; y < SCREEN_HEIGHT; y += PIX_MULTI*PIX_TILE) 
				SDL_RenderDrawLine(pApp->renderer, 0, y, SCREEN_WIDTH, y);
	  }

    // Sleep
    if ((pApp->frameLock) > (SDL_GetTicks() - pApp->frameStartTime)) {
      SDL_Delay((pApp->frameLock) - (SDL_GetTicks() - pApp->frameStartTime));
    }

  } while (GameEventStep(pApp, &Game));

  CleanQuit(pApp);
  return 0;
}
