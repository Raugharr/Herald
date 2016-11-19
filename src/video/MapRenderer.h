/*
 * File: MapRenderer.h
 * Author: David Brotz
 */
#ifndef __MAPRENDERER_H
#define __MAPRENDERER_H

#include "QuadTree.h"
#include "AABB.h"

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
	struct Tile** Tiles;
	uint32_t TileLength;
	uint32_t TileArea;
	struct QuadTree RenderArea[MAPRENDER_LAYERS];
	struct TileSheet TileSheets[8];
	SDL_Rect Screen;
	SDL_Texture* Grass;
	SDL_Texture* OddGrass;
	SDL_Texture* Selector;
	SDL_Texture* Settlement;
	SDL_Texture* Warrior;
	bool IsRendering;
};

struct MapRenderer* CreateMapRenderer(int _MapLength, SDL_Point* _RenderSize);
void MapLoad(struct MapRenderer* _Map);
/**
 * Finds the tile that corrisponds with the screen position given by _Screen.
 */
struct Tile* ScreenToTile(struct MapRenderer* _Map, const SDL_Point* _Screen);
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

#endif
