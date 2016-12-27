/*
 * File: Tile.h
 * Author: David Brotz
 */
#ifndef __TILE_H
#define __TILE_H

#include <SDL2/SDL.h>

typedef struct SDL_Texture SDL_Texture;
struct Resource;
struct MapRenderer;

extern const SDL_Point g_TileEvenOffsets[];
extern const SDL_Point g_TileOddOffsets[];

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

struct Tile {
	uint8_t Soil;//How fertile the soil is.
	uint8_t TileVar; //Which variation the will render.
	uint8_t Temperature;
	uint8_t TileSheet;
	uint8_t Terrain;
};

struct Tile* CreateTile(struct MapRenderer* _Renderer, uint8_t _TileSheet, uint8_t _TileVar, int _X, int _Y);
void DestroyTile(struct Tile* _Tile);
/**
 * Fills _Offset with the absolute position of the tile facing _Direction away from _Tile.
 */
void TileAdjTileOffset(const struct SDL_Point* _Tile, int _Direction, SDL_Point* _Offset);
int TileGetDistance(const struct SDL_Point* _Start, const struct SDL_Point* _End);

#endif
