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

// VEC 2


Vec2 Normalise(const Vec2& v)
{
	float l = sqrt(v.x * v.x + v.y * v.y);
	return Vec2{ v.x / l, v.y / l };
}

Vec2 operator+(const Vec2& a, const Vec2& b)
{
	return Vec2{ a.x + b.x, a.y + b.y };
}

Vec2 operator-(const Vec2& a, const Vec2& b)
{
	return Vec2{ a.x - b.x, a.y - b.y };
}


// Load BMP

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

void FloodHouse(uint16_t* pixs, std::list<HousePoint>& house, int x, int y,
                int w, int h) {
  if ((y < 0) || (x < 0) || (x > (w - 1)) || (y > (h - 1))) return;

  int c = x + y * w;
  if ((pixs[c] & 0xF) != 0xF) return;
  
  if (pixs[c] == 0xFFFF) {
	  house.push_back(HousePoint{ x, y, true });
  }
  else {
	  house.push_back(HousePoint{ x, y, false });
  }
  
  pixs[c] = 0xF00A;

  FloodHouse(pixs, house, x - 1, y - 1, w, h);
  FloodHouse(pixs, house, x, y - 1, w, h);
  FloodHouse(pixs, house, x + 1, y - 1, w, h);
  FloodHouse(pixs, house, x - 1, y, w, h);
  FloodHouse(pixs, house, x + 1, y, w, h);
  FloodHouse(pixs, house, x - 1, y + 1, w, h);
  FloodHouse(pixs, house, x, y + 1, w, h);
  FloodHouse(pixs, house, x + 1, y + 1, w, h);
}

std::list<std::list<HousePoint>> ExtractHouses(uint16_t* pixs, int w, int h) {
  int c = 0;
  std::list<std::list<HousePoint>> result;

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
		  std::list<HousePoint> house;
        FloodHouse(pixs, house, x, y, w, h);

        int numPts = 0;
		HousePoint* houseArr = new HousePoint[house.size()];
        houseArr[numPts++] = house.front();
        house.pop_front();

        while (house.empty() == false) {
			HousePoint e = houseArr[numPts - 1];
          auto bp = house.begin();
          int md =
              (e.x - bp->x) * (e.x - bp->x) + (e.y - bp->y) * (e.y - bp->y);
          for (auto p = house.begin(); p != house.end(); ++p) {
            int md_new =
                (e.x - p->x) * (e.x - p->x) + (e.y - p->y) * (e.y - p->y);
            if (md_new < md) {
              md = md_new;
              bp = p;
            }
          }

          houseArr[numPts++] = *bp;
          house.erase(bp);
        }

        // Get Angles
		HousePoint a = houseArr[numPts - 1];
		HousePoint b = houseArr[0];
		HousePoint c = houseArr[1];
        int di = 0;
        for (int i = 0; i < numPts; ++i) {
          if (a.isDoor && b.isDoor && c.isDoor) {
            b = c;
            c = houseArr[(i + 2) % numPts];
          } else if (!a.isDoor && b.isDoor) {
            houseArr[di].x = houseArr[i].x;
            houseArr[di].y = houseArr[i].y;
            houseArr[di++] = houseArr[i];

            a = b;
            b = c;
            c = houseArr[(i + 2) % numPts];
          } else if (a.isDoor && !b.isDoor) {
			houseArr[di] = houseArr[di-1];
            houseArr[di++].isDoor = false;

            a = b;
            b = c;
            c = houseArr[(i + 2) % numPts];
          } else if ((a.isDoor != b.isDoor) || (b.isDoor != c.isDoor)) {
            houseArr[di++] = houseArr[i];

            a = b;
            b = c;
            c = houseArr[(i + 2) % numPts];
		  }
		  else {
			  if ((abs(b.x - a.x) + abs(b.y - a.y))< 2) {
				  b = c;
				  c = houseArr[(i + 2) % numPts];
				  continue;
			  }
			  
			  Vec2 dxA = Normalise(Vec2{ c.x - b.x, c.y - b.y });
			  Vec2 dxB = Normalise(Vec2{ b.x - a.x, b.y - a.y });

			  Vec2 dxC = dxA - dxB;

			  // SDL_Log("%+.01f:%+.01f  \t %+.01f:%+.01f  \t %+.01f:%+.01f", dxA.x,
			  // dxA.y, dxB.x, dxB.y, dxC.x, dxC.y);

			  if (((dxC.x * dxC.x) + (dxC.y * dxC.y)) > 0.6) {
				  houseArr[di++] = houseArr[i];

				  a = b;
				  b = c;
				  c = houseArr[(i + 2) % numPts];
			  }
			  else {
				  b = c;
				  c = houseArr[(i + 2) % numPts];
			  }
		  }
        }
		
		std::list<HousePoint> sortedHouseList;
        for (int i = 0; i < di; ++i) {
          sortedHouseList.push_back(houseArr[i]);
        }
        delete houseArr;

        SDL_Log("House with %d points", sortedHouseList.size());
        result.push_back(sortedHouseList);
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
