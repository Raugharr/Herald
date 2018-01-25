/*
 * File: Tile.h
 * Author: David Brotz
 */
#ifndef __TILE_H
#define __TILE_H

#include "../sys/Log.h"

#include <SDL2/SDL.h>

typedef struct SDL_Texture SDL_Texture;
typedef uint16_t TileAx;
struct Resource;
struct MapRenderer;

#define TileFarmable(Tile) ((Tile)->Farmable* MILE_ACRE / 255)
#define TilePasturable(Tile) ((Tile)->Pasturable * MILE_ACRE / 255)

extern const SDL_Point g_TileEvenOffsets[];
extern const SDL_Point g_TileOddOffsets[];
extern const SDL_Point g_TileOffsets[];

enum ETileDir {
	TILE_NORTHWEST,
	TILE_NORTHEAST,
	TILE_EAST,
	TILE_SOUTHEAST,
	TILE_SOUTHWEST,
	TILE_WEST,
	TILE_SIZE
};

enum ETileTerrain {
	TILE_TGRASS,
	TILE_TBOG,
	TILE_TFOREST,
	TILE_THILL,
	TILE_TMOUNTAIN,
	TILE_TSIZE
};

struct TilePoint {
	TileAx x;
	TileAx y;
};

struct TileBase {
    uint8_t MoveCost;
};

struct Tile {
	uint8_t Farmable;//Percentage of this land that can be farmed.
	uint8_t Pasturable;//Percent of this land that can be put to pasture.
	uint8_t TileVar; //Which variation the will render.
	uint8_t TileSheet;
	uint8_t Terrain;
	uint32_t Trees;
};

struct CubeCoord {
	int32_t q;
	int32_t r;
	int32_t s;
};

struct Tile* CreateTile(struct MapRenderer* _Renderer, uint8_t _TileSheet, uint8_t _TileVar, int _X, int _Y);
void DestroyTile(struct Tile* _Tile);
/**
 * Fills _Offset with the absolute position of the tile facing _Direction away from _Tile.
 */
void TileAdjTileOffset(const struct SDL_Point* _Tile, int _Direction, SDL_Point* _Offset);
int TileDistance(const struct SDL_Point* _Start, const struct SDL_Point* _End);
static inline int CubeDistance(const struct CubeCoord* One, const struct CubeCoord* Two) {
	return (abs(One->q - Two->q) + abs(One->r - Two->r) + abs(One->s - Two->s)) / 2;
}
static inline void OffsetToCubeCoord(uint32_t x, uint32_t y, int32_t* q, int32_t* r, int32_t* s) {
	(*q) = y - (x - (x & 1)) / 2;
	(*r) = x;
	(*s) = -(*q) -(*r);
	Assert((*q) + (*r) + (*s) == 0);
}

static inline void HexToTile(uint32_t* x, uint32_t* y, const struct CubeCoord* Hex) {
	*x = Hex->q + (Hex->r - (Hex->r & 1)) / 2;
	*y = Hex->r;
}

static inline void HexMult(struct CubeCoord* Hex, int32_t Mult) {
	Hex->q *= Mult;
	Hex->r *= Mult;
	Hex->s *= Mult;	
}

static inline void TileNeighbor(int Direction, const SDL_Point* Tile, SDL_Point* Out) {
	SDL_Point Off = g_TileOffsets[Direction + (((Tile->y & 1) == 1) * 6)];
	Assert(Direction < TILE_SIZE);
	
	Out->x = Tile->x + Off.x;	
	Out->y = Tile->y + Off.y;	
	/*if((Tile->y & 1) == 0) {
		Out->x = Tile->x + g_TileEvenOffsets[Direction].x;	
		Out->y = Tile->y + g_TileEvenOffsets[Direction].y;	
	} else {
		Out->x = Tile->x + g_TileOddOffsets[Direction].x;	
		Out->y = Tile->y + g_TileOddOffsets[Direction].y;	
	}*/
}

static inline void TileOffset(int Direction, const SDL_Point* Tile, SDL_Point* Out) {
	SDL_Point Off = g_TileOffsets[((Tile->y & 1) == 0) * 6];
	
	Out->x = Off.x;	
	Out->y = Off.y;	

}
#endif
