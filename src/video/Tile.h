/*
 * File: Tile.h
 * Author: David Brotz
 */
#ifndef __TILE_H
#define __TILE_H

#include "Point.h"

typedef struct SDL_Texture SDL_Texture;
struct MapRenderer;

enum {
	TILE_NORTHWEST,
	TILE_NORTHEAST,
	TILE_EAST,
	TILE_SOUTHEAST,
	TILE_SOUTHWEST,
	TILE_WEST
};

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

#endif
