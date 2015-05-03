/*
 * File: Tile.h
 * Author: David Brotz
 */
#ifndef __TILE_H
#define __TILE_H

#define TILE_WIDTH (42)
#define TILE_WIDTH_THIRD (TILE_WIDTH * 0.75)
#define TILE_HEIGHT (48)
#define TILE_HEIGHT_THIRD (TILE_HEIGHT * 0.75)
#define TILE_GRADIENT ((TILE_HEIGHT / 4) / (TILE_WIDTH / 2))

enum {
	TILE_NORTHWEST,
	TILE_NORTHEAST,
	TILE_EAST,
	TILE_SOUTHEAST,
	TILE_SOUTHWEST,
	TILE_WEST
};

#include "Point.h"

typedef struct SDL_Texture SDL_Texture;
struct MapRenderer;

struct Tile {
	SDL_Texture* Image;
	struct Point TilePos;
	struct Point ScreenPos;
	float Forest;
	float Unbuildable;
	int Temperature;
};

struct Tile* CreateTile(struct MapRenderer* _Renderer, SDL_Texture* _Image, int _X, int _Y);
void DestroyTile(struct Tile* _Tile);

static inline const struct Point* TileGetScreenPos(const struct Tile* _Tile) {
	return &_Tile->ScreenPos;
}

#endif
