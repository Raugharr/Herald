/*
 * File: MapRenderer.h
 * Author: David Brotz
 */
#ifndef __MAPRENDERER_H
#define __MAPRENDERER_H

#include "QuadTree.h"
#include "AABB.h"
#include "Tile.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

#define TSHEET_TILES (25) 

struct Tile;
struct Army;
struct Settlement;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;

enum {
	MAPRENDER_TILE,
	MAPRENDER_SETTLEMENT,
	MAPRENDER_UNIT,
	MAPRENDER_LAYERS
};

struct TileSheet {
	const struct Resource* TileFile;
	SDL_Texture* Tiles;
	struct {
		uint16_t x;
		uint16_t y;	
	} VarPos[TSHEET_TILES];
};

struct MapRenderer {
	struct Tile* Tiles;
	uint32_t TileLength;
	uint32_t TileArea;
	struct QuadTree RenderArea[MAPRENDER_LAYERS];
	struct TileSheet TileSheets[8];
	SDL_Rect Screen;
	SDL_Texture* Grass;
	SDL_Texture* OddGrass;
	struct Resource* Selector;
	SDL_Texture* Settlement;
	SDL_Texture* Warrior;
	bool IsRendering;
};

struct MapRenderer* CreateMapRenderer(int _MapLength, SDL_Point* _RenderSize);
void MapLoad(struct MapRenderer* _Map);
/**
 * Finds the tile that corrisponds with the screen position given by _Screen.
 */
void ScreenToTile(const SDL_Point* _Screen, SDL_Point* _Hex);
void TileRing(struct MapRenderer* _Renderer, SDL_Point* _Center, uint32_t _Radius, struct Tile** _Out);
void TilesInRange(struct MapRenderer* _Renderer, const SDL_Point* _Pos, int _Range, struct LinkedList* _List);
/*
 * Inserts into _Rect the position and area of a game object at position _TilePos.
 */
void MapTileRenderRect(const struct MapRenderer* _Renderer, const SDL_Point* _TilePos, SDL_Rect* _Rect);
void MapRender(SDL_Renderer* _Renderer, struct MapRenderer* _Map);
void MapObjectsInRect(struct MapRenderer* _Renderer, int _Layer, const SDL_Rect* _Rect, struct LinkedList* _Data);
const struct Tile* MapGetTileConst(const struct MapRenderer* const _Renderer, const SDL_Point* _Point);
//FIXME: Move to Tile.h
struct Tile* MapGetTile(struct MapRenderer*  _Renderer, const SDL_Point* _Point);
void MapDrawColorOverlay(const struct MapRenderer* _Renderer, const SDL_Point* _Point, SDL_Color* _Color);

struct Army* MapGetUnit(struct MapRenderer* _Renderer, const SDL_Point* _Point);
int MapUnitCanMove(struct MapRenderer* _Renderer, struct Army* _Army, const SDL_Point* _Point);
int MapMoveUnit(struct MapRenderer* _Renderer, struct Army* _Army, const SDL_Point* _Point);

void MapDrawSettlementOverlay(struct MapRenderer* _Renderer, const struct Settlement* _Settlement);
static inline void TileNeighbor(int _Direction, SDL_Point* _Tile, SDL_Point* _Out) {
	SDL_Point _TDir;

	if((_Tile->y & 1) == 0) {
		_TDir.x = g_TileEvenOffsets[_Direction].x;	
		_TDir.y = g_TileEvenOffsets[_Direction].y;	
	} else {
		_TDir.x = g_TileOddOffsets[_Direction].x;	
		_TDir.y = g_TileOddOffsets[_Direction].y;	
	}
	_Out->x = _Tile->x + _TDir.x;
	_Out->y = _Tile->y + _TDir.y;
}

static inline void TileToPos(const struct MapRenderer* _Map, const struct Tile* _Tile, SDL_Point* _Out) {
	uint32_t _Offset  = (_Tile - &_Map->Tiles[0]) / sizeof(struct Tile);

	_Out->x = _Offset % _Map->TileLength;
	_Out->y = _Offset / _Map->TileLength;
}
#endif
