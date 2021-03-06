#include "SDL.h"
#include "SDL2_gfxPrimitives.h"
#include "global.h"
#include "sprite.h"
#include "game.h"
#include <algorithm>

enum TileId {
  TILE_ROAD,
  TILE_DIRT,
  TILE_BEACH,
  TILE_SEA,
  TILE_GRASS,
  TILE_FOREST,
  TILE_HOUSE,
  TILE_DOOR,
  TILE_LIGHT,
  NOOF_TILE
};

struct House {
  SDL_Point topLeft;
  SDL_Rect bounds;
  SDL_Texture* houseTex;
  SDL_Point* points;
  int numWallPts;
  int totalPts;
  bool isRender;
};

struct FloorTile {
  TileId id;
  int light;
};

//////////////////////////////////////////////////////////////////////////

static FloorTile* s_Tiles;
static std::list<House> s_houses;
static bool s_secondSweep = false;

bool isBlocking(const FloorTile& tilesDst) {
  return (tilesDst.id == TILE_HOUSE) || (tilesDst.id == TILE_SEA);
}

int StepLight(int c, int src, uint8_t lightStep) {
  switch (s_Tiles[c].id) {
    case TILE_ROAD:
      src -= lightStep * 4/5;
      break;
    case TILE_DIRT:
      src -= lightStep;
      break;
    case TILE_BEACH:
		src -= lightStep * 4 / 5;
      break;
    case TILE_SEA:
		src -= lightStep * 3 / 4;
      break;
    case TILE_GRASS:
      src -= lightStep;
      break;
    case TILE_FOREST:
      src -= lightStep * 3;
      break;
    case TILE_HOUSE:
      src = 0;
      break;
    case TILE_DOOR:
      src -= lightStep;
      break;
	case TILE_LIGHT:
		src -= lightStep;
  };
  return src;
}

void BuildHouse(SDL_Renderer* pRender,
                std::list<std::list<HousePoint>>::reference& oh) {
  House h;
  h.totalPts = oh.size() + 1;
  h.numWallPts = 0;
  int doorCurr = h.totalPts - 1;
  h.points = new SDL_Point[h.totalPts];
  h.bounds = SDL_Rect{100000, 1000000, 0, 0};
  for (auto p : oh) {
    SDL_Point pt =
        SDL_Point{p.x * PIX_TILE + PIX_HALF, p.y * PIX_TILE + PIX_HALF};

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
  filledPolygonRGBA(pRender, vx, vy, h.numWallPts, 60, 60, 60, 255);
  polygonRGBA(pRender, vx, vy, h.numWallPts, 0, 0, 0, 128);
  delete vx, vy;

  for (int i = h.numWallPts + 1; i < h.totalPts; i += 2) {
    thickLineRGBA(pRender, h.points[i - 1].x, h.points[i - 1].y, h.points[i].x,
                  h.points[i].y, 3, 255, 0, 255, 255);
  }

  SDL_RenderPresent(pRender);
  SDL_SetRenderTarget(pRender, NULL);
  h.isRender = true;

  //
  s_houses.push_back(h);
}

//////////////////////////////////////////////////////////////////////////
// Useful Small Functions

void GameState::DrawGridRect(SDL_Point& p, int w, int h, const SDL_Color& col) {
  SDL_SetRenderDrawColor(app->renderer, col.r, col.g, col.b, col.a);
  SDL_RenderFillRect(app->renderer,
                     &SDL_Rect{p.x - camScroll.x, p.y - camScroll.y, w, h});

  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 128);
  SDL_RenderDrawRect(app->renderer,
                     &SDL_Rect{p.x - camScroll.x, p.y - camScroll.y, w, h});
}

//////////////////////////////////////////////////////////////////////////
void GameState::StartGame(SDLAPP* _app) {
  app = _app;

  pGround = LoadBMP("ground.bmp");
  pHouse = LoadBMP("house.bmp");
  pGrad = LoadBMP("grad.bmp");

  // Player
  MapSize = pGround->clip_rect;
  camScroll = SDL_Point{500, 400};
  playerPos = SDL_Point{615, 495};

  // Setup Houses
  s_houses.clear();
  SDL_LockSurface(pHouse);
  auto housesList = ExtractHouses((uint16_t*)pHouse->pixels,
                                  pHouse->clip_rect.w, pHouse->clip_rect.h);
  SDL_UnlockSurface(pHouse);

  while (housesList.empty() == false) {
    auto oh = housesList.front();
    BuildHouse(app->renderer, oh);
    housesList.pop_front();
  }

  // Setup Light
  pTexLight =
      SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, MapSize.w, MapSize.h);
  SDL_SetTextureBlendMode(pTexLight, SDL_BLENDMODE_BLEND);

  // Setup Ground
  SDL_UnlockSurface(pGround);
  s_Tiles = new FloorTile[MapSize.w * MapSize.h];
  uint16_t* pix = (uint16_t*)pGround->pixels;
  int c = 0;
  for (int y = 0; y < pGround->h; ++y) {
    for (int x = 0; x < pGround->w; ++x) {
      uint16_t col = pix[c];
      switch (col) {
        case COL_ROAD:
          s_Tiles[c++] = FloorTile{TILE_ROAD, 8};
          break;
        case COL_DIRT:
          s_Tiles[c++] = FloorTile{TILE_DIRT, 8};
          break;
        case COL_BEACH:
          s_Tiles[c++] = FloorTile{TILE_BEACH, 8};
          break;
        case COL_SEA:
          s_Tiles[c++] = FloorTile{TILE_SEA, 8};
          break;
        case COL_GRASS:
          s_Tiles[c++] = FloorTile{TILE_GRASS, 8};
          break;
        case COL_FOREST:
          s_Tiles[c++] = FloorTile{TILE_FOREST, 8};
          break;
        case COL_ROOF:
        case COL_HOUSE:
          s_Tiles[c++] = FloorTile{TILE_HOUSE, 8};
          break;
        case COL_DOOR:
          s_Tiles[c++] = FloorTile{TILE_DOOR, 8};
          break;
		case COL_LIGHT:
			s_Tiles[c++] = FloorTile{ TILE_LIGHT, 8 };
			break;
        default:
          SDL_Log("COL [%d,%d] %x", x, y, col);
      }
    }
  }
  SDL_LockSurface(pGround);

  pTexLevel = SDL_CreateTextureFromSurface(app->renderer, pGround);

  UpdateLighting();
}

void GameState::Render() {
  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
  SDL_RenderClear(app->renderer);

  SDL_Rect tarRect = SDL_Rect{-camScroll.x, -camScroll.y, MapSize.w * PIX_TILE,
                              MapSize.h * PIX_TILE};

  SDL_RenderCopy(app->renderer, pTexLevel, &pGround->clip_rect, &tarRect);
  SDL_RenderCopy(app->renderer, pTexLight, &pGround->clip_rect, &tarRect);
  /*
  for (auto h : s_houses) {
          SDL_Rect r = h.bounds;
          r.x = h.topLeft.x - camScroll.x;
          r.y = h.topLeft.y - camScroll.y;
          SDL_RenderCopy(app->renderer, h.houseTex, &h.bounds, &r);
  }*/

  DrawGridRect(playerPos, PIX_TILE, PIX_TILE, SDL_Color{200, 0, 0, 255});

  SDL_RenderPresent(app->renderer);
}

void GameState::Update() {
  // Update Player
  SDL_Point playVel;
  {
    SDL_Point oldPos = playerPos;
    SDL_Point newPos{playerPos.x + ((buttonMap[B_LEFT] > 0) ? -1 : 0) +
                         ((buttonMap[B_RIGHT] > 0) ? +1 : 0),
                     playerPos.y + ((buttonMap[B_UP] > 0) ? -1 : 0) +
                         ((buttonMap[B_DOWN] > 0) ? +1 : 0)};

    UpdateMovePlayer(newPos);
    playVel = {playerPos.x - oldPos.x, playerPos.y - oldPos.y};
  }

  // Update Camera Scroll
  UpdateCamera(playVel);

  // Update Lighting
  // UpdateLighting();

  // Clear Buttons
  for (int b = NOOF_BUTTONS - 1; b >= 0; --b) {
    buttonMap[b] &= 1;
  }
}

void GameState::UpdateLighting() {
  uint32_t* pix;
  int pitch;
  SDL_LockTexture(pTexLight, NULL, (void**)(&pix), &pitch);

  int c = 0;
  for (int y = 0; y < MapSize.h; ++y) {
    for (int x = 0; x < MapSize.w; ++x) {
      switch (s_Tiles[c].id) {
        case TILE_HOUSE:
          pix[c] = 0x20;
          break;
        case TILE_DOOR:
          pix[c] = 0xE0;
          break;
		case TILE_LIGHT:
			pix[c] = 0xFF;
			break;
        default:
          pix[c] = 0;
          break;
      }

      c++;
    }
  }

  LightSweep(pix, 8);
  LightSweep(pix, 8);
  LightSweep(pix, 8);

  c = 0;
  for (int y = 0; y < MapSize.h; ++y) {
    for (int x = 0; x < MapSize.w; ++x) {
      uint32_t& pt = pix[c];
	  // STEP INTO GROUPS
	  pt = 0xC0 & ~pt;

	  /*
      if (pt < 25) {
        pt = (((25 - pt) * 3) << 8) + 0xDD;
      } else {
        //pt = 0xFF & ~pt;


      }*/
      c++;
    }
  }

  SDL_UnlockTexture(pTexLight);
}

void GameState::LightSweep(uint32_t* pixSrc, uint8_t lightStep) {
  int c;
  uint32_t* pix = pixSrc;

  // Sweep South
  c = MapSize.w;
  for (int y = MapSize.h - 2; y >= 0; --y) {
    for (int x = MapSize.w - 1; x >= 0; --x) {
      int src = pix[c - MapSize.w];
      src = StepLight(c, src, lightStep);

      uint32_t& tar = pix[c];
      if (src > (int)tar) tar = src;

      c++;
    }
  }

  // Sweep North
  c = MapSize.w * MapSize.h - MapSize.w - 1;
  for (int y = MapSize.h - 2; y >= 0; --y) {
    for (int x = MapSize.w - 1; x >= 0; --x) {
      int src = pix[c + MapSize.w];
      src = StepLight(c, src, lightStep);

      uint32_t& tar = pix[c];
      if (src > (int)tar) tar = src;

      c--;
    }
  }

  // Sweep East
  c = 0;
  for (int y = MapSize.h - 1; y >= 0; --y) {
    c++;
    for (int x = MapSize.w - 2; x >= 0; --x) {
      int src = pix[c - 1];
      src = StepLight(c, src, lightStep);

      uint32_t& tar = pix[c];
      if (src > (int)tar) tar = src;

      c++;
    }
  }

  // Sweep West
  c = MapSize.w * MapSize.h - 1;
  for (int y = MapSize.h - 1; y >= 0; --y) {
    for (int x = MapSize.w - 2; x >= 0; --x) {
      int src = pix[c + 1];
      src = StepLight(c, src, lightStep);

      uint32_t& tar = pix[c];
      if (src > (int)tar) tar = src;

      c--;
    }
    c--;
  }

  // Check Diag
  c = MapSize.w;
  int diagLightStep = lightStep*3/2;
  for (int y = MapSize.h - 3; y >= 0; --y) {
	  c++;
	  for (int x = MapSize.w - 3; x >= 0; --x) {
		  
		  pix[c] = std::max({ 
			  StepLight(c, pix[c - 1 + MapSize.w], diagLightStep),
			  StepLight(c, pix[c - 1 - MapSize.w], diagLightStep),
			  StepLight(c, pix[c + 1 + MapSize.w], diagLightStep),
			  StepLight(c, pix[c + 1 - MapSize.w], diagLightStep),
			  (int)pix[c] });
		  c++;
	  }
	  c++;
  }
}

void GameState::UpdateCamera(SDL_Point& playVel) {
  camScroll.x += ((buttonMap[B_CAMLEFT] > 0) ? -PIX_TILE : 0) +
                 ((buttonMap[B_CAMRIGHT] > 0) ? +PIX_TILE : 0);
  camScroll.y += ((buttonMap[B_CAMUP] > 0) ? -PIX_TILE : 0) +
                 ((buttonMap[B_CAMDOWN] > 0) ? +PIX_TILE : 0);

  SDL_Point camRelPos = {playerPos.x - camScroll.x - PIX_WIDTH / 2,
                         playerPos.y - camScroll.y - PIX_HEIGHT / 2};
  static SDL_Point prevVel{0, 0};

  if (playVel.x == 0)
    playVel.x = (camRelPos.x > 2) - (camRelPos.x < 2);
  else {
    if (abs(camRelPos.x) > PIX_WIDTH / 3) {
      playVel.x = 0;
      if (camRelPos.x > 0)
        camScroll.x = playerPos.x - PIX_WIDTH / 3 - PIX_WIDTH / 2 - 2;
      else
        camScroll.x = playerPos.x + PIX_WIDTH / 3 - PIX_WIDTH / 2 + 2;
    } else if (playVel.x > 0)
      playVel.x += 1 + (prevVel.x > 0);
    else if (playVel.x < 0)
      playVel.x -= 1 + (prevVel.x < 0);
  }

  if (playVel.y == 0)
    playVel.y = (camRelPos.y > 2) - (camRelPos.y < 2);
  else {
    if (abs(camRelPos.y) > PIX_HEIGHT / 3) {
      playVel.y = 0;
      if (camRelPos.y > 0)
        camScroll.y = playerPos.y - PIX_HEIGHT / 3 - PIX_HEIGHT / 2 - 2;
      else
        camScroll.y = playerPos.y + PIX_HEIGHT / 3 - PIX_HEIGHT / 2 + 2;
    } else if (playVel.y > 0)
      playVel.y += 1 + (prevVel.y > 0);
    else if (playVel.y < 0)
      playVel.y -= 1 + (prevVel.y < 0);
  }

  prevVel = playVel;

  camScroll.x += playVel.x;
  camScroll.y += playVel.y;

  BoundToMap(camScroll, PIX_WIDTH, PIX_HEIGHT);
}

bool GameState::UpdateMovePlayer(SDL_Point newPos) {
  BoundToMap(newPos, PIX_TILE, PIX_TILE);

  if ((newPos.x == playerPos.x) && (newPos.y == playerPos.y)) return false;

  // Tiles under player
  int tiles[8] = {0};
  int aC = GetTiles(playerPos, &tiles[0]);
  int bC = GetTiles(newPos, &tiles[aC]) + aC;

  bool isBlocked = false;
  for (int i = aC; !isBlocked && (i < bC); ++i) {
    isBlocked |= isBlocking(s_Tiles[tiles[i]]);
  }

  if (isBlocked) {
    if ((newPos.x != playerPos.x) &&
        UpdateMovePlayer(SDL_Point{playerPos.x, newPos.y}))
      return true;

    if ((newPos.y != playerPos.y) &&
        UpdateMovePlayer(SDL_Point{newPos.x, playerPos.y}))
      return true;

    return false;
  }

  playerPos = newPos;
  return true;
}

void GameState::BoundToMap(SDL_Point& pt, int w, int h) const {
  if (pt.x < 0) pt.x = 0;
  if (pt.y < 0) pt.y = 0;
  if (pt.x > MapSize.w * PIX_TILE - w) pt.x = MapSize.w * PIX_TILE - w;
  if (pt.y > MapSize.h * PIX_TILE - h) pt.y = MapSize.h * PIX_TILE - h;
}

int GameState::GetTiles(const SDL_Point& pt, int* fourInts) const {
  SDL_Point ptA = SDL_Point{pt.x / PIX_TILE, pt.y / PIX_TILE};
  SDL_Point ptB = SDL_Point{ptA.x + (((pt.x % PIX_TILE) > 0) ? 1 : 0),
                            ptA.y + (((pt.y % PIX_TILE) > 0) ? 1 : 0)};
  BoundToMap(ptA, PIX_TILE, PIX_TILE);
  BoundToMap(ptB, PIX_TILE, PIX_TILE);

  int total = 0;
  fourInts[total++] = ptA.x + ptA.y * MapSize.w;
  if (ptA.x != ptB.x) fourInts[total++] = ptB.x + ptA.y * MapSize.w;
  if (ptA.y != ptB.y) fourInts[total++] = ptA.x + ptB.y * MapSize.w;
  if (total == 3) fourInts[total++] = ptB.x + ptB.y * MapSize.w;

  return total;
}

void GameState::GameEvent(SDL_Event* evt) {
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

        case SDL_SCANCODE_I:
          buttonMap[B_CAMUP] = 3;
          break;
        case SDL_SCANCODE_K:
          buttonMap[B_CAMDOWN] = 3;
          break;
        case SDL_SCANCODE_J:
          buttonMap[B_CAMLEFT] = 3;
          break;
        case SDL_SCANCODE_L:
          buttonMap[B_CAMRIGHT] = 3;
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

        case SDL_SCANCODE_S:
          s_secondSweep = !s_secondSweep;
          break;

        case SDL_SCANCODE_I:
          buttonMap[B_CAMUP] = 0;
          break;
        case SDL_SCANCODE_K:
          buttonMap[B_CAMDOWN] = 0;
          break;
        case SDL_SCANCODE_J:
          buttonMap[B_CAMLEFT] = 0;
          break;
        case SDL_SCANCODE_L:
          buttonMap[B_CAMRIGHT] = 0;
          break;
      }
      break;

    case SDL_MOUSEMOTION:
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
      auto pt = SDL_Point{(evt->button.x / PIX_MULTI) + camScroll.x,
                          (evt->button.y / PIX_MULTI) + camScroll.y};

      pt.x = pt.x - pt.x % PIX_TILE;
      pt.y = pt.y - pt.y % PIX_TILE;
      playerPos = pt;

      SDL_Log("Mouse %d,%d -> %d,%d", evt->button.x, evt->button.y, playerPos.x,
              playerPos.y);
      for (auto h : s_houses) {
      }
    }

    case SDL_MOUSEWHEEL:
      break;
  }
}

//
