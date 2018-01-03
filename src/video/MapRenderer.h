/*
 * File: MapRenderer.h
 * Author: David Brotz
 */
#ifndef __MAPRENDERER_H
#define __MAPRENDERER_H

#include "QuadTree.h"
#include "AABB.h"
#include "Tile.h"
#include "Sprite.h"

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

enum {
	MAPRENMODE_STATE,
	MAPRENMODE_DIPLOMATIC,
	MAPRENMODE_SIZE
};

//Influence Score.
struct InfScore {
	uint32_t SetId : 24;
	uint32_t Score : 8;
};

struct TileSheet {
	const struct Resource* TileFile;
	SDL_Texture* Tiles;
	struct {
		TileAx x;
		TileAx y;	
	} VarPos[TSHEET_TILES];
};

struct MapRenderer {
	struct Tile* Tiles;
	uint32_t TileLength;
	uint32_t TileArea;
	struct QuadTree RenderArea[MAPRENDER_LAYERS];
	struct TileSheet TileSheets[8];
	SDL_Rect Screen;
	struct Resource* Selector;
	struct Resource* Settlement;
	SDL_Texture* Warrior;
	uint8_t RenderMode; //Use MAPRENMODE*
	bool IsRendering;
};

struct MapRenderer* CreateMapRenderer(int MapLength, SDL_Point* RenderSize);
void MapLoad(struct MapRenderer* Map);
/**
 * Finds the tile that corrisponds with the screen position given by Screen.
 */
void ScreenToHex(const SDL_Point* Screen, SDL_Point* Hex);
void TileRing(const struct MapRenderer* Renderer, const SDL_Point* Center, uint16_t Radius, struct Tile** Out);
void TileSpiral(const struct MapRenderer* Renderer, const SDL_Point* Center, uint16_t Radius, struct Tile** Out);
/**
 * Returns the number of tiles that exist in a given radius.
 */
static inline uint32_t NumTileRadius(uint16_t Radius) {
	return 3 * (Radius * Radius) - (3 * Radius) + 1;
}
void TilesInRange(struct MapRenderer* Renderer, const SDL_Point* Pos, int Range, struct LinkedList* List);
/*
 * Inserts into Rect the position and area of a game object at position TilePos.
 */
void MapTileRenderRect(const struct MapRenderer* Renderer, const SDL_Point* TilePos, SDL_Rect* Rect);
static inline void MapHexOffset(uint16_t ScreenX, uint16_t ScreenY, uint16_t Y, uint16_t* Out) {
	*(Out) += (~(ScreenY) & 1) * (Y & 1) * (TILE_WIDTH / 2); //If map screen is even then add (TILE_WIDTH / 2) if tile y pos is odd.
	*(Out) += (ScreenY & 1) * (~(Y & 1) & 1) * (TILE_WIDTH / 2);//If map screen is odd then add (TILE_WIDTH / 2) if tile y pos is even.
}
void MapRender(SDL_Renderer* Renderer, SDL_Texture* Texture, uint16_t SrcX, uint16_t SrcY, uint16_t DestX, uint16_t DestY);
void MapRenderAll(SDL_Renderer* Renderer, struct MapRenderer* Map);
/**
 *\out Data Size
 */
void MapObjectsInRect(struct MapRenderer* Renderer, uint8_t Layer, const SDL_Rect* Rect, void** Data, uint32_t* Size, uint32_t TableSz);
const struct Tile* MapGetTileConst(const struct MapRenderer* const Renderer, const SDL_Point* Point);
//FIXME: Move to Tile.h
struct Tile* MapGetTile(struct MapRenderer*  Renderer, const SDL_Point* Point);
void MapDrawColorOverlay(const struct MapRenderer* Renderer, const SDL_Point* Point, SDL_Color* Color);

struct Army* MapGetUnit(struct MapRenderer* Renderer, const SDL_Point* Point);
bool MapUnitCanMove(struct MapRenderer* Renderer, struct Army* Army, const SDL_Point* Point);
int MapMoveUnit(struct MapRenderer* Renderer, struct Army* Army, const SDL_Point* Point);
void MapZeroArea(uint8_t* Map, uint16_t Width, int32_t Posx, int32_t Posy);
uint8_t MapGravBest(uint8_t* Map, TileAx Width, TileAx Posx, TileAx Posy, TileAx* Bestx, TileAx* Besty);

void CenterScreen(struct MapRenderer* Renderer, uint32_t x, uint32_t y);

void MapDrawSettlementOverlay(struct MapRenderer* Renderer, const struct Settlement* Settlement);

static inline void TileToPos(const struct MapRenderer* Map, const struct Tile* Tile, SDL_Point* Out) {
	uint32_t Offset  = (Tile - &Map->Tiles[0]);

	Out->x = Offset % Map->TileLength;
	Out->y = Offset / Map->TileLength;
}

static inline uint32_t TileToIndex(const struct MapRenderer* Map, const struct Tile* Tile) {
	return (Tile - &Map->Tiles[0]);
}
struct InfScore* CalcInfluence(const struct Settlement** SetList, uint32_t GovSz, struct MapRenderer* Renderer);
#endif
