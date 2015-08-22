#include "SDL.h"
#include <iostream>

#include "gif.h"

#define SCREEN_TITLE "LudumDare #33 - @EvilKimau"
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FRAME_RATE 30

enum Buttons { B_UP, B_DOWN, B_LEFT, B_RIGHT, NOOF_BUTTONS };

struct SDLAPP {
  SDL_Window *m_window;
  SDL_Renderer *m_renderer;

  SDL_Event m_LastEvent;
  Uint32 m_startTime;
  Uint32 m_frameDelta;
  Uint32 m_frameStartTime;
  Uint32 m_frameLock;

  int m_buttonMap[NOOF_BUTTONS];
};

SDLAPP *CreateApp() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
	  SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "SDL_Init");
    return nullptr;
  }

  SDLAPP *pApp = new SDLAPP();

  pApp->m_startTime = SDL_GetTicks();
  pApp->m_frameLock = 1000 / FRAME_RATE;

  pApp->m_window = SDL_CreateWindow(SCREEN_TITLE, 100, 100, SCREEN_WIDTH,
                                    SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (pApp->m_window == nullptr) {
	  SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateWindow");
    delete pApp;
    return nullptr;
  }

  pApp->m_renderer = SDL_CreateRenderer(
      pApp->m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (pApp->m_renderer == nullptr) {
	  SDL_LogError(SDL_LOG_CATEGORY_RENDER, "SDL_CreateRenderer");
    SDL_DestroyWindow(pApp->m_window);
    delete pApp;
    return nullptr;
  }

  return pApp;
}

void CleanQuit(SDLAPP *pApp) {
  SDL_DestroyRenderer(pApp->m_renderer);
  SDL_DestroyWindow(pApp->m_window);
  SDL_Quit();
}

void RenderTestScene(uint16_t *pixs, SDL_Rect srcRect) {
  uint16_t GBAColours[4] = {0x141, 0x363, 0x9B1, 0xAC1};

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      pixs[x + y * SCREEN_WIDTH] = GBAColours[y % 4];
    }

    pixs[x] = 0x0FFF;
    pixs[x + (SCREEN_HEIGHT / 2) * SCREEN_WIDTH] = 0x0F00;
  }
}

int GameEventStep(SDLAPP *app) {
  int newTime = SDL_GetTicks();
  app->m_frameDelta = newTime - app->m_frameStartTime;
  app->m_frameStartTime = newTime;

  // Events
  auto evt = &app->m_LastEvent;
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

	  case SDL_MOUSEBUTTONUP:
	  {
		  auto pt = SDL_Point{ evt->button.x, evt->button.y };
		  SDL_Log("Mouse %d,%d", pt.x, pt.y);
	  }
                  break;

      case SDL_KEYDOWN:
        switch (evt->key.keysym.scancode) {
          case SDL_SCANCODE_UP:
            app->m_buttonMap[B_UP] = 3;
            break;
          case SDL_SCANCODE_DOWN:
            app->m_buttonMap[B_DOWN] = 3;
            break;
          case SDL_SCANCODE_LEFT:
            app->m_buttonMap[B_LEFT] = 3;
            break;
          case SDL_SCANCODE_RIGHT:
            app->m_buttonMap[B_RIGHT] = 3;
            break;
        }
        break;

      case SDL_KEYUP:
        switch (evt->key.keysym.scancode) {
          case SDL_SCANCODE_UP:
            app->m_buttonMap[B_UP] = 0;
            break;
          case SDL_SCANCODE_DOWN:
            app->m_buttonMap[B_DOWN] = 0;
            break;
          case SDL_SCANCODE_LEFT:
            app->m_buttonMap[B_LEFT] = 0;
            break;
          case SDL_SCANCODE_RIGHT:
            app->m_buttonMap[B_RIGHT] = 0;
            break;
        }
        break;

      case SDL_QUIT:
        SDL_Log("QUIT");
        return 0;

      default:
        SDL_Log("Event: %d", evt->type);
    }

    // TODO :: Handle Event
  }

  // Update

  // TODO :: Update

  return 1;
}

uint8_t *decomGif;

void Render(SDLAPP *pApp) {
  SDL_SetRenderDrawColor(pApp->m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(pApp->m_renderer);

  // TODO :: Render
  SDL_SetRenderDrawColor(pApp->m_renderer, 200, 0, 0, 255);
  SDL_RenderDrawLine(pApp->m_renderer, 10, 10, 200, 200);

  SDL_RenderPresent(pApp->m_renderer);
}

void Update(SDLAPP *pApp) {
  // TODO :: Game Update
  pApp;
}

int main(int argc, char *argv[]) {
  SDLAPP *pApp = CreateApp();
  if (pApp == nullptr) {
    return -1;
  }

  if (GameEventStep(pApp) == 0) {
    CleanQuit(pApp);
    return 0;
  }

  do {
    Update(pApp);
    Render(pApp);

    // Sleep
    if ((pApp->m_frameLock) > (SDL_GetTicks() - pApp->m_frameStartTime)) {
      SDL_Delay((pApp->m_frameStartTime) -
                (SDL_GetTicks() - pApp->m_frameStartTime));
    }

  } while (GameEventStep(pApp));

  CleanQuit(pApp);
  return 0;
}
