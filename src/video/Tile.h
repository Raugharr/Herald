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

enum {
	TILE_NORTHWEST,
	TILE_NORTHEAST,
	TILE_EAST,
	TILE_SOUTHEAST,
	TILE_SOUTHWEST,
	TILE_WEST,
	TILE_SIZE
};

struct Tile {
	struct Resource* Image;
	SDL_Rect Rect; //Area of sprite to render.
	SDL_Rect SpritePos;//Where to render sprite.
	SDL_Point TilePos;
	float Forest;
	float Unbuildable;
	int Temperature;
};

struct Tile* CreateTile(struct MapRenderer* _Renderer, struct Resource* _Image, int _X, int _Y);
void DestroyTile(struct Tile* _Tile);
void GetAdjPos(const SDL_Point* _Pos, SDL_Point* _Adj, int _TileDir);
struct Tile* GetAdjTile(struct MapRenderer* _Map, const struct Tile* _Tile, int _TileDir);
/**
 * Fills _AdjTiles will every adjacent tile of _Tile. _AdjTiles should be an array of struct Tile* that is TILE_SIZE in length.
 */
void TileGetAdjTiles(struct MapRenderer* _Renderer, const struct Tile* _Tile, struct Tile** _AdjTiles);
/**
 * Fills _Offset with the absolute position of the tile facing _Direction away from _Tile.
 */
void TileAdjTileOffset(const struct SDL_Point* _Tile, int _Direction, SDL_Point* _Offset);
int TileGetDistance(const struct SDL_Point* _Start, const struct SDL_Point* _End);
/*
 * Fills _Adj with the next point in the ring and returns the direction of _Adj in relation to _Point.
 * _Point the current point in the ring.
 * _Adj the point that will be filled with the next point in the ring.
 * _RingUsed how much of the current ring have been iterated over already.
 * _Radius how far this ring is from the center of the spiral ring.
 */
int TileNextInRing(const SDL_Point* _Point, SDL_Point* _Adj, int _RingUsed, int _Radius);
int TileNextRing(const SDL_Point* _Point, SDL_Point* _New, int _Radius);

#endif
