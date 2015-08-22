#include <unordered_map>
#include <array>
#include <stdio.h>
#include "SDL.h"
#include "sprite.h"

struct SpriteData {
  enum Anchor {
    TOP_LEFT,
    TOP,
    TOP_RIGHT,
    LEFT,
    MIDDLE,
    RIGHT,
    BOTTOM_LEFT,
    BOTTOM,
    BOTTOM_RIGHT
  };

  std::vector<SDL_Rect> sprites;
  std::string name;
  int sprSheetNum;
};

static std::unordered_map<std::string, SpriteData*> s_SpriteMap;
static std::list<SDL_Surface*> s_Surfaces;

int SetupSprites(uint16_t* pixs, SDL_Rect* pxSz, int sprSheetNum);

SDL_Surface* LoadBMP(const char* filename) {
  SDL_Surface* loadSurf = SDL_LoadBMP(filename);
  if (loadSurf == 0) {
    SDL_Log(SDL_GetBasePath());
    SDL_Log(SDL_GetError());
    return 0;
  }
  SDL_Surface* newSurf =
      SDL_ConvertSurfaceFormat(loadSurf, SDL_PIXELFORMAT_RGBA4444, 0);
  SDL_FreeSurface(loadSurf);
  return newSurf;
}

void FloodHouse(uint16_t* pixs, std::list<SDL_Point>& house, int x, int y,
                int w, int h) {
  if ((y < 0) || (x < 0) || (x > (w - 1)) || (y > (h - 1))) return;

  int c = x + y * w;
  if ((pixs[c] & 0xF) != 0xF) 
	  return;
  
  pixs[c] = 0xF00A;
  house.push_back(SDL_Point{x, y});

  FloodHouse(pixs, house, x - 1, y - 1, w, h);
  FloodHouse(pixs, house, x, y - 1, w, h);
  FloodHouse(pixs, house, x - 1, y, w, h);
  FloodHouse(pixs, house, x + 1, y, w, h);
  FloodHouse(pixs, house, x, y + 1, w, h);
  FloodHouse(pixs, house, x + 1, y + 1, w, h);
}

std::list<std::list<SDL_Point>> ExtractHouses(uint16_t* pixs, int w, int h) {
  int c = 0;
  std::list<std::list<SDL_Point>> result;

  uint16_t houseCol = 0x000F;
  uint16_t doorCol = 0xFFFF;

  // Clear Confusion
  c = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if ((pixs[c] != houseCol) && (pixs[c] != doorCol)) {
        pixs[c] = 0;
      }
      ++c;
    }
  }

  // Edge Find
  c = 0;
  for (int y = 1; y < (h - 1); y++) {
    for (int x = 1; x < (w - 1); x++) {
      if ((pixs[c] != 0) &&
          !((pixs[c - w] == 0) || (pixs[c - 1] == 0) || (pixs[c + 1] == 0) ||
            (pixs[c + w] == 0))) {
        pixs[c] = 0xF003;
      }
      ++c;
    }
  }

  c = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if ((pixs[c] & 0xF) == 0xF) {
        // Scan for House
        std::list<SDL_Point> house;
        FloodHouse(pixs, house, x, y, w, h);

        SDL_Log("House with %d points", house.size());
        result.push_back(house);
      }
      ++c;
    }
  }

  return result;
}

int SetupSpritesFromFile(const char* filename) {
  SDL_Surface* newSprSurf = LoadBMP(filename);

  int total = SetupSprites((uint16_t*)newSprSurf->pixels,
                           &newSprSurf->clip_rect, s_Surfaces.size());

  if (total > 0)
    s_Surfaces.push_back(newSprSurf);
  else
    SDL_FreeSurface(newSprSurf);
  return total;
}

int SetupSprites(uint16_t* pixs, SDL_Rect* pxSz, int sprSheetNum) {
  int numSpritesTotal = 0;
  std::list<SDL_Rect> corners;
  uint16_t framePix = pixs[0];

  // Gather Corners
  for (int y = 0; y < pxSz->h; ++y) {
    for (int x = 0; x < pxSz->w; ++x) {
      if ((pixs[x + y * pxSz->w] == framePix) &&
          (pixs[x + 1 + y * pxSz->w] == framePix) &&
          (pixs[x + (y + 1) * pxSz->w] == framePix) &&
          (pixs[(x + 1) + (y + 1) * pxSz->w] == 0)) {
        corners.push_back({x, y});
      }
    }
  }

  for (SDL_Rect p : corners) {
    // Build Sprite
    std::list<SDL_Rect> sprStrip;
    SpriteData* newSprite = new SpriteData;

    {
      char buff[100];
      sprintf_s(buff, "_SPR %d - %d:%d", sprSheetNum, p.x, p.y);
      newSprite->name = buff;
    }

    newSprite->sprSheetNum = sprSheetNum;

    // Scroll Wide First
    SDL_Rect currRect{p.x + 2, p.y + 2, 0, 0};

    int yoffset = p.y * pxSz->w;
    for (int x = p.x + 2; x < pxSz->w; ++x) {
      if ((pixs[x + yoffset] == framePix) &&
          (pixs[x + yoffset + pxSz->w] == 0)) {
        if (currRect.w == 0) currRect.x = x;
        ++currRect.w;
      } else if ((pixs[x + yoffset] == 0) &&
                 (pixs[x + yoffset + pxSz->w] == 0)) {
        if (currRect.w > 0) {
          sprStrip.push_back(currRect);
          currRect.w = 0;
        }
      } else {
        break;
      }
    }

    // Get Height
    int sprH = 0;

    int xoffset = p.x;
    for (int y = p.y + 2; y < pxSz->h; ++y) {
      if ((pixs[xoffset + y * pxSz->w] == framePix) &&
          (pixs[xoffset + 1 + y * pxSz->w] == 0)) {
        ++sprH;
      } else if ((pixs[xoffset + y * pxSz->w] == 0) &&
                 (pixs[xoffset + 1 + y * pxSz->w] == 0)) {
        if (sprH > 0) {
          for (SDL_Rect r : sprStrip) {
            r.y = y - sprH;
            r.h = sprH;
            newSprite->sprites.push_back(r);
            ++numSpritesTotal;
          }

          sprH = 0;
        }
      } else {
        break;
      }
    }

    s_SpriteMap[newSprite->name] = newSprite;

    // All Done
    SDL_Log("[%d:%d] = %d", p.x, p.y, newSprite->sprites.size());
  }

  return numSpritesTotal;
}
