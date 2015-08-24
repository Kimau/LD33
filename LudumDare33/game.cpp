#include "SDL.h"
#include "SDL2_gfxPrimitives.h"
#include "global.h"
#include "sprite.h"
#include "game.h"


enum TileId {
	TILE_ROAD,
	TILE_DIRT,
	TILE_BEACH,
	TILE_SEA,
	TILE_GRASS,
	TILE_FOREST,
	TILE_HOUSE,
	TILE_DOOR,
	NOOF_TILE
};

struct FloorTile {
	TileId id;
	int light;
};

bool isBlocking(const FloorTile& tilesDst) {
	return (tilesDst.id == TILE_HOUSE) || (tilesDst.id == TILE_SEA);
}


static FloorTile* s_Tiles;

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

void GameState::DrawGridRect(SDL_Point &p, int w, int h, const SDL_Color &col) {
  SDL_SetRenderDrawColor(app->renderer, col.r, col.g, col.b, col.a);
  SDL_RenderFillRect(app->renderer, &SDL_Rect{p.x - camScroll.x,
                                              p.y - camScroll.y, w, h});

  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 128);
  SDL_RenderDrawRect(app->renderer, &SDL_Rect{p.x - camScroll.x,
                                              p.y - camScroll.y, w, h});
}

//////////////////////////////////////////////////////////////////////////
void GameState::StartGame(SDLAPP *_app) {
  app = _app;

  pGround = LoadBMP("ground.bmp");
  pHouse = LoadBMP("house.bmp");
  pGrad = LoadBMP("grad.bmp");

  // Player
  MapSize = pGround->clip_rect;
  camScroll = SDL_Point{ 500, 400 };
  playerPos = SDL_Point{ 615, 495 };

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
  SDL_UnlockSurface(pGround);
  s_Tiles = new FloorTile[MapSize.w*MapSize.h];
  uint16_t* pix = (uint16_t*)pGround->pixels;
  int c = 0;
  for (int y = 0; y < pGround->h; ++y)
  {
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
                  default:
					  SDL_Log("COL [%d,%d] %x", x,y, col);
		  }
	  }
  }
  SDL_LockSurface(pGround);

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

	DrawGridRect(playerPos, PIX_TILE, PIX_TILE, SDL_Color{ 200, 0, 0, 255 });

	SDL_RenderPresent(app->renderer);
}



void GameState::Update() {
  // Update Player
  {
	  SDL_Point newPos{
		  playerPos.x + ((buttonMap[B_LEFT] > 0) ? -1 : 0) +
		  ((buttonMap[B_RIGHT] > 0) ? +1 : 0),
		  playerPos.y + ((buttonMap[B_UP] > 0) ? -1 : 0) +
		  ((buttonMap[B_DOWN] > 0) ? +1 : 0) };

	  UpdateMovePlayer(newPos);

  }

  // Update Camera Scroll
  static Vec2 camVel{ 0, 0 };
  int dx = ((buttonMap[B_CAMLEFT] > 0) ? -PIX_TILE : 0) + ((buttonMap[B_CAMRIGHT] > 0) ? +PIX_TILE : 0);
  int dy = ((buttonMap[B_CAMUP] > 0) ? -PIX_TILE : 0) + ((buttonMap[B_CAMDOWN] > 0) ? +PIX_TILE : 0);
  if (dx != 0)
	  camVel.x = dx*1.0f;
  if (dy != 0)
	  camVel.y = dy*1.0f;

  SDL_Point camRelPos = { playerPos.x - camScroll.x, playerPos.y - camScroll.y };

  if (camRelPos.x < PIX_WIDTH/4) {
	  if (camRelPos.x < PIX_WIDTH/5)
		  camVel.x = -1.0f;
	  else if (camVel.x > 0)
		  camVel.x = 0;
  }
  if (camRelPos.x >(PIX_WIDTH - PIX_WIDTH/4)) {
	  if (camRelPos.x > (PIX_WIDTH - PIX_WIDTH / 5))
		  camVel.x = +1.0f;
	  else if (camVel.x < 0)
		  camVel.x = 0;
  }
  if (camRelPos.y < PIX_HEIGHT/4) {
	  if (camRelPos.y < PIX_HEIGHT/5)
		  camVel.y = -1.0f;
	  else if (camVel.y > 0)
		  camVel.y = 0;
  }
  if (camRelPos.y > (PIX_HEIGHT - PIX_HEIGHT/4)) {
	  if (camRelPos.y >(PIX_HEIGHT - PIX_HEIGHT/5))
		  camVel.y = +1.0f;
	  else if (camVel.y < 0)
		  camVel.y = 0;
  }

  if (camVel.x > 0) camScroll.x += PIX_TILE*4/3;
  if (camVel.x < 0) camScroll.x -= PIX_TILE * 4 / 3;
  if (camVel.y > 0) camScroll.y += PIX_TILE * 4 / 3;
  if (camVel.y < 0) camScroll.y -= PIX_TILE * 4 / 3;

  BoundToMap(camScroll, PIX_WIDTH, PIX_HEIGHT);

  // Clear Buttons
  for (int b = NOOF_BUTTONS - 1; b >= 0; --b) {
    buttonMap[b] &= 1;
  }
}

bool GameState::UpdateMovePlayer(SDL_Point newPos) {
  BoundToMap(newPos, PIX_TILE, PIX_TILE);

  if((newPos.x == playerPos.x) && (newPos.y == playerPos.y))
	  return false;

  // Tiles under player
  int tiles[8] = {0};
  int aC = GetTiles(playerPos, &tiles[0]);
  int bC = GetTiles(newPos, &tiles[aC]) + aC;

  bool isBlocked = false;
  for (int i = aC; !isBlocked && (i < bC); ++i) {
    isBlocked |= isBlocking(s_Tiles[tiles[i]]);
  }

  if (isBlocked) {
    if((newPos.x != playerPos.x) && UpdateMovePlayer(SDL_Point{playerPos.x, newPos.y}))
        return true;
     
	if ((newPos.y != playerPos.y) && UpdateMovePlayer(SDL_Point{ newPos.x, playerPos.y }))
        return true;

    return false;
  }

  playerPos = newPos;
  return true;
}

void GameState::BoundToMap(SDL_Point& pt, int w, int h) const
{
	if (pt.x < 0) pt.x = 0;
	if (pt.y < 0) pt.y = 0;
	if (pt.x > MapSize.w*PIX_TILE - w)
		pt.x = MapSize.w*PIX_TILE - w;
	if (pt.y > MapSize.h*PIX_TILE - h)
		pt.y = MapSize.h*PIX_TILE - h;
}

int GameState::GetTiles(const SDL_Point &pt, int *fourInts) const {
	SDL_Point ptA = SDL_Point{ pt.x / PIX_TILE, pt.y / PIX_TILE };
	SDL_Point ptB = SDL_Point{ ptA.x + (((pt.x%PIX_TILE)>0) ? 1 : 0), ptA.y + (((pt.y%PIX_TILE)>0) ? 1 : 0) };
	BoundToMap(ptA, PIX_TILE, PIX_TILE);
	BoundToMap(ptB, PIX_TILE, PIX_TILE);

	int total = 0;
	fourInts[total++] = ptA.x + ptA.y * MapSize.w;
	if (ptA.x != ptB.x) fourInts[total++] = ptB.x + ptA.y * MapSize.w;
	if (ptA.y != ptB.y) fourInts[total++] = ptA.x + ptB.y * MapSize.w;
	if (total == 3) fourInts[total++] = ptB.x + ptB.y * MapSize.w;

	return total;
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
    case SDL_MOUSEBUTTONUP:
	{
		
		auto pt = SDL_Point{ 
			(evt->button.x / PIX_MULTI) + camScroll.x,
			(evt->button.y / PIX_MULTI) + camScroll.y };

		pt.x = pt.x - pt.x % PIX_TILE;
		pt.y = pt.y - pt.y % PIX_TILE;
		playerPos = pt;

		SDL_Log("Mouse %d,%d -> %d,%d", evt->button.x, evt->button.y, playerPos.x, playerPos.y);
		for (auto h : s_houses) {

		}
	}
	
    case SDL_MOUSEWHEEL:  break;
  }
}

//
