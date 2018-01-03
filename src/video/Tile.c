/*
 * File: Tile.c
 * Author: David Brotz
 */

#include "Tile.h"

#include "MapRenderer.h"
#include "QuadTree.h"
#include "Sprite.h"

#ifdef DEBUG
#include "../sys/Log.h"
#endif

#include <SDL2/SDL.h>
#include <stdlib.h>

const SDL_Point g_TileEvenOffsets[] = {
		{-1, -1},
		{0, -1},
		{1, 0},
		{0, 1},
		{-1, 1},
		{-1, 0}
};

const SDL_Point g_TileOddOffsets[] = {
		{0, -1},
		{1, -1},
		{1, 0},
		{1, 1},
		{0, 1},
		{-1, 0}
};


const SDL_Point g_TileOffsets[] = {
		{-1, -1},
		{0, -1},
		{1, 0},
		{0, 1},
		{-1, 1},
		{-1, 0},
		{0, -1},
		{1, -1},
		{1, 0},
		{1, 1},
		{0, 1},
		{-1, 0}
};

struct Tile* CreateTile(struct MapRenderer* Renderer, uint8_t TileSheet, uint8_t TileVar, int X, int Y) {
	struct Tile* Tile = (struct Tile*) malloc(sizeof(struct Tile));

	Tile->TileSheet = TileSheet;
	Tile->TileVar = TileVar;
	return Tile;
}

void DestroyTile(struct Tile* Tile) {
	free(Tile);
}

int TileDistance(const struct SDL_Point* Start, const struct SDL_Point* End) {
	//return sign(1);
	return (abs(Start->x - End->x) + abs(Start->y - End->y) + abs((-Start->x - Start->y) - (-End->x - End->y))) / 2;
}
