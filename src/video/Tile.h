/*
 * File: Tile.h
 * Author: David Brotz
 */
#ifndef __TILE_H
#define __TILE_H

#define TILE_WIDTH (64)
#define TILE_HEIGHT (64)

#define TILE_NORTHWEST(_Pos) _Pos.Y -= 1
#define TILE_NORTHEAST(_Pos) _Pox.X += 1; _Pos.Y -= 1
#define TILE_EAST(_Pos) _Pos.X += 1
#define TILE_SOUTHEAST(_Pos) _Pos.Y += 1
#define TILE_SOUTHWEST(_Pos) _Pos.X -= 1; _Pos.Y += 1
#define TILE_WEST(_Pos) _Pos.X -= 1

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
