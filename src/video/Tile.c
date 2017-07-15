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

struct Tile* CreateTile(struct MapRenderer* _Renderer, uint8_t _TileSheet, uint8_t _TileVar, int _X, int _Y) {
	struct Tile* _Tile = (struct Tile*) malloc(sizeof(struct Tile));

	_Tile->Temperature = 0;
	_Tile->TileSheet = _TileSheet;
	_Tile->TileVar = _TileVar;
	return _Tile;
}

void DestroyTile(struct Tile* _Tile) {
	free(_Tile);
}

void TileAdjTileOffset(const struct SDL_Point* _Tile, int _Direction, SDL_Point* _Pos) {
	if(_Direction >= TILE_SIZE) {
		_Pos->x = _Tile->x;
		_Pos->y = _Tile->y;
		return;
	}
	if((_Tile->y & 1) == 0) {
		_Pos->x = _Tile->x + g_TileEvenOffsets[_Direction].x;
		_Pos->y = _Tile->y + g_TileEvenOffsets[_Direction].y;
	} else {
		_Pos->x = _Tile->x + g_TileOddOffsets[_Direction].x;
		_Pos->y = _Tile->y + g_TileOddOffsets[_Direction].y;
	}
}

int TileDistance(const struct SDL_Point* _Start, const struct SDL_Point* _End) {
	//return sign(1);
	return (abs(_Start->x - _End->x) + abs(_Start->y - _End->y) + abs((-_Start->x - _Start->y) - (-_End->x - _End->y))) / 2;
}
